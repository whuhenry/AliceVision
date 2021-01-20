// This file is part of the AliceVision project.
// Copyright (c) 2017 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/system/Logger.hpp>
#include <aliceVision/mvsData/Point3d.hpp>
#include <aliceVision/mvsData/Rgb.hpp>
#include <aliceVision/mvsData/StaticVector.hpp>
#include <aliceVision/mvsData/Voxel.hpp>
#include <aliceVision/mvsUtils/common.hpp>
#include <aliceVision/mesh/Mesh.hpp>
#include <aliceVision/fuseCut/delaunayGraphCutTypes.hpp>
#include <aliceVision/fuseCut/VoxelsGrid.hpp>

#include <geogram/delaunay/delaunay.h>
#include <geogram/delaunay/delaunay_3d.h>
#include <geogram/mesh/mesh.h>
#include <geogram/basic/geometry_nd.h>

#include <map>
#include <set>

namespace aliceVision {

namespace sfmData {
class SfMData;
}

namespace fuseCut {


struct FuseParams
{
    /// Max input points loaded from images
    int maxInputPoints = 50000000;
    /// Max points at the end of the depth maps fusion
    int maxPoints = 5000000;
    /// The step used to load depth values from depth maps is computed from maxInputPts. Here we define the minimal value for this step,
    /// so on small datasets we will not spend too much time at the beginning loading all depth values.
    int minStep = 2;
    /// After fusion, filter points based on their number of observations
    int minVis = 2;

    float simFactor = 15.0f;
    float angleFactor = 15.0f;
    double pixSizeMarginInitCoef = 2.0;
    double pixSizeMarginFinalCoef = 1.0;
    float voteMarginFactor = 4.0f;
    float contributeMarginFactor = 2.0f;
    float simGaussianSizeInit = 10.0f;
    float simGaussianSize = 10.0f;
    double minAngleThreshold = 0.1;
    bool refineFuse = true;
};


class DelaunayGraphCut
{
public:
    using VertexIndex = GEO::index_t;
    using CellIndex = GEO::index_t;

    struct Facet
    {
        Facet(){}
        Facet(CellIndex ci, VertexIndex lvi)
            : cellIndex(ci)
            , localVertexIndex(lvi)
        {}

        CellIndex cellIndex = GEO::NO_CELL;
        /// local opposite vertex index
        VertexIndex localVertexIndex = GEO::NO_VERTEX;
    };

    mvsUtils::MultiViewParams* mp;

    GEO::Delaunay_var _tetrahedralization;
    /// 3D points coordinates
    std::vector<Point3d> _verticesCoords;
    /// Information attached to each vertex
    std::vector<GC_vertexInfo> _verticesAttr;
    /// Information attached to each cell
    std::vector<GC_cellInfo> _cellsAttr;
    /// isFull info per cell: true is full / false is empty
    std::vector<bool> _cellIsFull;

    std::vector<int> _camsVertexes;
    std::vector<std::vector<CellIndex>> _neighboringCellsPerVertex;

    bool saveTemporaryBinFiles;

    static const GEO::index_t NO_TETRAHEDRON = GEO::NO_CELL;

    DelaunayGraphCut(mvsUtils::MultiViewParams* _mp);
    virtual ~DelaunayGraphCut();

    /**
     * @brief Retrieve the global vertex index of the localVertexIndex of the facet.
     * 
     * @param f the facet
     * @return the global vertex index
     */
    inline VertexIndex getOppositeVertexIndex(const Facet& f) const
    {
        return _tetrahedralization->cell_vertex(f.cellIndex, f.localVertexIndex);
    }

    /**
     * @brief Retrieve the global vertex index of a vertex from a facet and an relative index 
     * compared to the localVertexIndex of the facet.
     * 
     * @param f the facet
     * @param i the relative index (relative to the localVertexIndex of the facet)
     * @return the global vertex index
     */
    inline VertexIndex getVertexIndex(const Facet& f, int i) const
    {
        return _tetrahedralization->cell_vertex(f.cellIndex, ((f.localVertexIndex + i + 1) % 4));
    }

    inline const std::array<const Point3d*, 3> getFacetsPoints(const Facet& f) const
    {
        return {&(_verticesCoords[getVertexIndex(f, 0)]),
                &(_verticesCoords[getVertexIndex(f, 1)]),
                &(_verticesCoords[getVertexIndex(f, 2)])};
    }

    inline std::size_t getNbVertices() const
    {
        return _verticesAttr.size();
    }

    inline GEO::index_t nearestVertexInCell(GEO::index_t cellIndex, const Point3d& p) const
    {
        GEO::signed_index_t result = NO_TETRAHEDRON;
        double d = std::numeric_limits<double>::max();
        for(GEO::index_t i = 0; i < 4; ++i)
        {
            GEO::signed_index_t currentVertex = _tetrahedralization->cell_vertex(cellIndex, i);
            if(currentVertex < 0)
                continue;
            double currentDist = GEO::Geom::distance2(_verticesCoords[currentVertex].m, p.m, 3);
            if(currentDist < d)
            {
                d = currentDist;
                result = currentVertex;
            }
        }
        return result;
    }

    inline GEO::index_t locateNearestVertex(const Point3d& p) const
    {
        if(_tetrahedralization->nb_vertices() == 0)
            return GEO::NO_VERTEX;
        /*
        GEO::index_t cellIndex = ((const GEO::Delaunay3d*)_tetrahedralization.get())->locate(p.m); // TODO GEOGRAM: how to??
        if(cellIndex == NO_TETRAHEDRON)
            return GEO::NO_VERTEX;

        return nearestVertexInCell(cellIndex, p);
        */
        return _tetrahedralization->nearest_vertex(p.m); // TODO GEOGRAM: this is a brute force approach!
    }

    /**
     * @brief A cell is infinite if one of its vertices is infinite.
     */
    inline bool isInfiniteCell(CellIndex ci) const
    {
        return _tetrahedralization->cell_is_infinite(ci);
        // return ci < 0 || ci > getNbVertices();
    }
    inline bool isInvalidOrInfiniteCell(CellIndex ci) const
    {
        return ci == GEO::NO_CELL || isInfiniteCell(ci);
        // return ci < 0 || ci > getNbVertices();
    }

    inline Facet mirrorFacet(const Facet& f) const
    {
        const std::array<VertexIndex, 3> facetVertices = {
            getVertexIndex(f, 0),
            getVertexIndex(f, 1),
            getVertexIndex(f, 2)
        };
        
        Facet out;
        out.cellIndex = _tetrahedralization->cell_adjacent(f.cellIndex, f.localVertexIndex);
        if(out.cellIndex != GEO::NO_CELL)
        {
            // Search for the vertex in adjacent cell which doesn't exist in input facet.
            for(int k = 0; k < 4; ++k)
            {
                CellIndex out_vi = _tetrahedralization->cell_vertex(out.cellIndex, k);
                if(std::find(facetVertices.begin(), facetVertices.end(), out_vi) == facetVertices.end())
                {
                  out.localVertexIndex = k;
                  return out;
                }
            }
        }
        return out;
    }

    void updateVertexToCellsCache()
    {
        _neighboringCellsPerVertex.clear();

        std::map<VertexIndex, std::set<CellIndex>> neighboringCellsPerVertexTmp;
        int coutInvalidVertices = 0;
        for(CellIndex ci = 0, nbCells = _tetrahedralization->nb_cells(); ci < nbCells; ++ci)
        {
            for(VertexIndex k = 0; k < 4; ++k)
            {
                CellIndex vi = _tetrahedralization->cell_vertex(ci, k);
                if(vi == GEO::NO_VERTEX || vi >= _verticesCoords.size())
                {
                    ++coutInvalidVertices;
                    continue;
                }
                neighboringCellsPerVertexTmp[vi].insert(ci);
            }
        }
        ALICEVISION_LOG_INFO("coutInvalidVertices: " << coutInvalidVertices);
        ALICEVISION_LOG_INFO("neighboringCellsPerVertexTmp: " << neighboringCellsPerVertexTmp.size());
        _neighboringCellsPerVertex.resize(_verticesCoords.size());
        ALICEVISION_LOG_INFO("verticesCoords: " << _verticesCoords.size());
        for(const auto& it: neighboringCellsPerVertexTmp)
        {
            const std::set<CellIndex>& input = it.second;
            std::vector<CellIndex>& output = _neighboringCellsPerVertex[it.first];
            output.assign(input.begin(), input.end());
        }
    }

    /**
     * @brief vertexToCells
     *
     * It is a replacement for GEO::Delaunay::next_around_vertex which doesn't work as expected.
     *
     * @param vi
     * @param lvi
     * @return global index of the lvi'th neighboring cell
     */
    CellIndex vertexToCells(VertexIndex vi, int lvi) const
    {
        const std::vector<CellIndex>& localCells = _neighboringCellsPerVertex.at(vi);
        if(lvi >= localCells.size())
            return GEO::NO_CELL;
        return localCells[lvi];
    }

    /**
     * @brief Retrieves the global indexes of neighboring cells using the global index of a vertex.
     * 
     * @param vi the global vertexIndex
     * @return a vector of neighboring cell indexes
     */
    inline const std::vector<CellIndex>& getNeighboringCellsByVertexIndex(VertexIndex vi) const
    {
        return _neighboringCellsPerVertex.at(vi);
    }

    void initVertices();
    void computeDelaunay();
    void initCells();
    void displayStatistics();

    void saveDhInfo(const std::string& fileNameInfo);
    void saveDh(const std::string& fileNameDh, const std::string& fileNameInfo);

    StaticVector<StaticVector<int>*>* createPtsCams();
    void createPtsCams(StaticVector<StaticVector<int>>& out_ptsCams);
    StaticVector<int>* getPtsCamsHist();
    StaticVector<int>* getPtsNrcHist();
    StaticVector<int> getIsUsedPerCamera() const;
    StaticVector<int> getSortedUsedCams() const;

    void addPointsFromSfM(const Point3d hexah[8], const StaticVector<int>& cams, const sfmData::SfMData& sfmData);
    void addPointsFromCameraCenters(const StaticVector<int>& cams, float minDist);
    void addPointsToPreventSingularities(const Point3d Voxel[8], float minDist);

    /**
     * @brief Add volume points to prevent singularities
     */
    void addHelperPoints(int nGridHelperVolumePointsDim, const Point3d Voxel[8], float minDist);

    void fuseFromDepthMaps(const StaticVector<int>& cams, const Point3d voxel[8], const FuseParams& params);

    void computeVerticesSegSize(bool allPoints, float alpha = 0.0f);
    void removeSmallSegs(int minSegSize);

    bool rayCellIntersection(const Point3d& camCenter, const Point3d& p, const Facet& inFacet, Facet& outFacet,
                             bool nearestFarest, Point3d& outIntersectPt) const;

    Facet getFacetFromVertexOnTheRayToTheCam(VertexIndex globalVertexIndex, int cam, bool nearestFarest) const;
    Facet getFirstFacetOnTheRayFromCamToThePoint(int cam, const Point3d& p, Point3d& intersectPt) const;

    float distFcn(float maxDist, float dist, float distFcnHeight) const;

    inline double conj(double val) const { return val; }
    double facetMaxEdgeLength(Facet& f1) const;
    double maxEdgeLength() const;
    Point3d cellCircumScribedSphereCentre(CellIndex ci) const;
    double getFaceWeight(const Facet &f1) const;

    float weightFcn(float nrc, bool labatutWeights, int ncams);

    virtual void fillGraph(bool fixesSigma, float nPixelSizeBehind, bool labatutWeights,
                           bool fillOut, float distFcnHeight = 0.0f);
    void fillGraphPartPtRc(int& out_nstepsFront, int& out_nstepsBehind, int vertexIndex, int cam, float weight,
                           bool fixesSigma, float nPixelSizeBehind, bool fillOut,
                           float distFcnHeight);

    void forceTedgesByGradientIJCV(bool fixesSigma, float nPixelSizeBehind);

    int setIsOnSurface();

    void addToInfiniteSw(float sW);

    void freeUnwantedFullCells(const Point3d* hexah);

    void reconstructGC(const Point3d* hexah);

    void maxflow();

    void voteFullEmptyScore(const StaticVector<int>& cams, const std::string& folderName);

    void createDensePointCloud(Point3d hexah[8], const StaticVector<int>& cams, const sfmData::SfMData* sfmData, const FuseParams* depthMapsFuseParams);

    void createGraphCut(Point3d hexah[8], const StaticVector<int>& cams, const std::string& folderName, const std::string& tmpCamsPtsFolderName, bool removeSmallSegments);

    /**
     * @brief Invert full/empty status of cells if they represent a too small group after labelling.
     */
    void invertFullStatusForSmallLabels();

    void graphCutPostProcessing();

    void segmentFullOrFree(bool full, StaticVector<int>** inColors, int& nsegments);
    int removeBubbles();
    int removeDust(int minSegSize);
    void leaveLargestFullSegmentOnly();

    mesh::Mesh* createMesh(bool filterHelperPointsTriangles = true);
    mesh::Mesh* createTetrahedralMesh() const;
};

} // namespace fuseCut
} // namespace aliceVision
