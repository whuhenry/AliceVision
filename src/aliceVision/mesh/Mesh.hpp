// This file is part of the AliceVision project.
// Copyright (c) 2017 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/mvsData/Matrix3x3.hpp>
#include <aliceVision/mvsData/Point2d.hpp>
#include <aliceVision/mvsData/Point3d.hpp>
#include <aliceVision/mvsData/Rgb.hpp>
#include <aliceVision/mvsData/StaticVector.hpp>
#include <aliceVision/mvsData/Voxel.hpp>
#include <aliceVision/mvsUtils/common.hpp>

#include <geogram/points/kd_tree.h>

namespace aliceVision {
namespace mesh {

using PointVisibility = StaticVector<int>;
using PointsVisibility = StaticVector<PointVisibility>;

class Mesh
{
public:

    struct triangle
    {
        int v[3]; ///< vertex indexes
        bool alive;

        triangle()
        {
            v[0] = -1;
            v[1] = -1;
            v[2] = -1;
            alive = true;
        }

        triangle(int a, int b, int c)
        {
            v[0] = a;
            v[1] = b;
            v[2] = c;
            alive = true;
        }

        triangle& operator=(const triangle& other)
        {
            v[0] = other.v[0];
            v[1] = other.v[1];
            v[2] = other.v[2];
            alive = other.alive;
            return *this;
        }
    };

    struct triangle_proj
    {
        Point2d tp2ds[3];
        Pixel tpixs[3];
        Pixel lu, rd;

        triangle_proj& operator=(const triangle_proj param)
        {
            tp2ds[0] = param.tp2ds[0];
            tp2ds[1] = param.tp2ds[1];
            tp2ds[2] = param.tp2ds[2];
            tpixs[0] = param.tpixs[0];
            tpixs[1] = param.tpixs[1];
            tpixs[2] = param.tpixs[2];
            lu = param.lu;
            rd = param.rd;
            return *this;
        }
    };

    struct rectangle
    {
        Point2d P[4];
        Point2d lu, rd;

        rectangle(Pixel cell, int scale)
        {
            P[0].x = (float)(cell.x * scale + 0);
            P[0].y = (float)(cell.y * scale + 0);
            P[1].x = (float)(cell.x * scale + scale);
            P[1].y = (float)(cell.y * scale + 0);
            P[2].x = (float)(cell.x * scale + scale);
            P[2].y = (float)(cell.y * scale + scale);
            P[3].x = (float)(cell.x * scale + 0);
            P[3].y = (float)(cell.y * scale + scale);
            lu = P[0];
            rd = P[2];
        }

        rectangle& operator=(const rectangle param)
        {
            for(int i = 0; i < 4; i++)
            {
                P[i] = param.P[i];
            };
            lu = param.lu;
            rd = param.rd;
            return *this;
        }
    };

protected:
    /// Per-vertex color data
    std::vector<rgb> _colors;
    /// Per triangle material id
    std::vector<int> _trisMtlIds;

public:
    StaticVector<Point3d> pts;
    StaticVector<Mesh::triangle> tris;

    int nmtls = 0;
    StaticVector<Point2d> uvCoords;
    StaticVector<Voxel> trisUvIds;
    StaticVector<Point3d> normals;
    StaticVector<Voxel> trisNormalsIds;
    PointsVisibility pointsVisibilities;

    Mesh();
    ~Mesh();

    void saveToObj(const std::string& filename);

    bool loadFromBin(const std::string& binFileName);
    void saveToBin(const std::string& binFileName);
    bool loadFromObjAscii(const std::string& objAsciiFileName);

    void addMesh(const Mesh& mesh);

    void getTrisMap(StaticVector<StaticVector<int>>& out, const mvsUtils::MultiViewParams& mp, int rc, int scale, int w, int h);
    void getTrisMap(StaticVector<StaticVector<int>>& out, StaticVector<int>& visTris, const mvsUtils::MultiViewParams& mp, int rc, int scale,
                    int w, int h);
    /// Per-vertex color data const accessor
    const std::vector<rgb>& colors() const { return _colors; }
    /// Per-vertex color data accessor
    std::vector<rgb>& colors() { return _colors; }

    /// Per-triangle material ids const accessor
    const std::vector<int>& trisMtlIds() const { return _trisMtlIds; }
    std::vector<int>& trisMtlIds() { return _trisMtlIds; }

    void getDepthMap(StaticVector<float>& depthMap, const mvsUtils::MultiViewParams& mp, int rc, int scale, int w, int h);
    void getDepthMap(StaticVector<float>& depthMap, StaticVector<StaticVector<int>>& tmp, const mvsUtils::MultiViewParams& mp, int rc,
                     int scale, int w, int h);

    void getPtsNeighbors(std::vector<std::vector<int>>& out_ptsNeighTris) const;
    void getPtsNeighborTriangles(StaticVector<StaticVector<int>>& out_ptsNeighTris) const;
    void getPtsNeighPtsOrdered(StaticVector<StaticVector<int>>& out_ptsNeighTris) const;

    void getVisibleTrianglesIndexes(StaticVector<int>& out_visTri, const std::string& tmpDir, const mvsUtils::MultiViewParams& mp, int rc, int w, int h);
    void getVisibleTrianglesIndexes(StaticVector<int>& out_visTri, const std::string& depthMapFileName, const std::string& trisMapFileName,
                                                  const mvsUtils::MultiViewParams& mp, int rc, int w, int h);
    void getVisibleTrianglesIndexes(StaticVector<int>& out_visTri, StaticVector<StaticVector<int>>& trisMap,
                                                  StaticVector<float>& depthMap, const mvsUtils::MultiViewParams& mp, int rc, int w,
                                                  int h);
    void getVisibleTrianglesIndexes(StaticVector<int>& out_visTri, StaticVector<float>& depthMap, const mvsUtils::MultiViewParams& mp, int rc, int w,
                                                  int h);

    void generateMeshFromTrianglesSubset(const StaticVector<int>& visTris, Mesh& outMesh, StaticVector<int>& out_ptIdToNewPtId) const;

    void getNotOrientedEdges(StaticVector<StaticVector<int>>& edgesNeighTris, StaticVector<Pixel>& edgesPointsPairs);
    void getTrianglesEdgesIds(const StaticVector<StaticVector<int>>& edgesNeighTris, StaticVector<Voxel>& out) const;

    void getLaplacianSmoothingVectors(StaticVector<StaticVector<int>>& ptsNeighPts, StaticVector<Point3d>& out_nms,
                                      double maximalNeighDist = -1.0f);
    void laplacianSmoothPts(float maximalNeighDist = -1.0f);
    void laplacianSmoothPts(StaticVector<StaticVector<int>>& ptsNeighPts, double maximalNeighDist = -1.0f);
    void computeNormalsForPts(StaticVector<Point3d>& out_nms);
    void computeNormalsForPts(StaticVector<StaticVector<int>>& ptsNeighTris, StaticVector<Point3d>& out_nms);
    void smoothNormals(StaticVector<Point3d>& nms, StaticVector<StaticVector<int>>& ptsNeighPts);
    Point3d computeTriangleNormal(int idTri);
    Point3d computeTriangleCenterOfGravity(int idTri) const;
    double computeTriangleMaxEdgeLength(int idTri) const;
    double computeTriangleMinEdgeLength(int idTri) const;

    void removeFreePointsFromMesh(StaticVector<int>& out_ptIdToNewPtId);

    void letJustTringlesIdsInMesh(StaticVector<int>& trisIdsToStay);

    double computeAverageEdgeLength() const;
    double computeLocalAverageEdgeLength(const std::vector<std::vector<int>>& ptsNeighbors, int ptId) const;

    bool isTriangleAngleAtVetexObtuse(int vertexIdInTriangle, int triId) const;
    bool isTriangleObtuse(int triId) const;

public:
    double computeTriangleProjectionArea(const triangle_proj& tp) const;
    double computeTriangleArea(int idTri) const;
    Mesh::triangle_proj getTriangleProjection(int triid, const mvsUtils::MultiViewParams& mp, int rc, int w, int h) const;
    bool isTriangleProjectionInImage(const mvsUtils::MultiViewParams& mp, const Mesh::triangle_proj& tp, int camId, int margin) const;
    int getTriangleNbVertexInImage(const mvsUtils::MultiViewParams& mp, const Mesh::triangle_proj& tp, int camId, int margin) const;
    bool doesTriangleIntersectsRectangle(Mesh::triangle_proj& tp, Mesh::rectangle& re);
    void getTrianglePixelIntersectionsAndInternalPoints(Mesh::triangle_proj& tp, Mesh::rectangle& re, StaticVector<Point2d>& out);
    void getTrianglePixelIntersectionsAndInternalPoints(const mvsUtils::MultiViewParams& mp, int idTri, Pixel& pix,
                                                              int rc, Mesh::triangle_proj& tp, Mesh::rectangle& re,
                                                              StaticVector<Point3d>& out);

    Point2d getTrianglePixelInternalPoint(Mesh::triangle_proj& tp, Mesh::rectangle& re);

    int subdivideMesh(const Mesh& refMesh, float ratioSubdiv, bool remapVisibilities);
    int subdivideMeshOnce(const Mesh& refMesh, const GEO::AdaptiveKdTree& refMesh_kdTree, float ratioSubdiv);

    void computeTrisCams(StaticVector<StaticVector<int>>& trisCams, const mvsUtils::MultiViewParams& mp, const std::string tmpDir);
    void computeTrisCamsFromPtsCams(StaticVector<StaticVector<int>>& trisCams) const;

    void initFromDepthMap(const mvsUtils::MultiViewParams& mp, float* depthMap, int rc, int scale, int step, float alpha);
    void initFromDepthMap(const mvsUtils::MultiViewParams& mp, StaticVector<float>& depthMap, int rc, int scale, float alpha);
    void initFromDepthMap(int stepDetail, const mvsUtils::MultiViewParams& mp, float* depthMap, int rc, int scale, int step,
                          float alpha);
    void removeTrianglesInHexahedrons(StaticVector<Point3d>* hexahsToExcludeFromResultingMesh);
    void removeTrianglesOutsideHexahedron(Point3d* hexah);
    void filterLargeEdgeTriangles(double cutAverageEdgeLengthFactor);
    void invertTriangleOrientations();
    void changeTriPtId(int triId, int oldPtId, int newPtId);
    int getTriPtIndex(int triId, int ptId, bool failIfDoesNotExists = true) const;
    Pixel getTriOtherPtsIds(int triId, int _ptId) const;
    bool areTwoTrisSameOriented(int triId1, int triId2, int edgePtId1, int edgePtId2) const;
    void getLargestConnectedComponentTrisIds(StaticVector<int>& out) const;

    bool getEdgeNeighTrisInterval(Pixel& itr, Pixel& edge, StaticVector<Voxel>& edgesXStat,
                                  StaticVector<Voxel>& edgesXYStat);
};

} // namespace mesh
} // namespace aliceVision
