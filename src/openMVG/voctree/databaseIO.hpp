#pragma once

#include "database.hpp"
#include "vocabulary_tree.hpp"

#include <openMVG/features/descriptor.hpp>
#include <openMVG/sfm/sfm_data_io.hpp>

#include <string>

namespace openMVG {
namespace voctree {

/**
 * @brief Given an non empty database, it queries the database with a set of images
 * and their associated features and returns, for each image, the first \p numResults best
 * matching documents in the database
 * 
 * @param[in] fileFullPath A file containing the path the features to load, it could be a .txt or an OpenMVG .json
 * @param[in] tree The vocabulary tree to be used for feature quantization
 * @param[in] db The built database
 * @param[in] numResults The number of results to retrieve for each image
 * @param[out] allMatches The matches for all the images
 * @param[out] documents For each document, it contains the list of associated visual words 
 */
template<class DescriptorT>
void queryDatabase(const std::string &fileFullPath,
                   const openMVG::voctree::VocabularyTree<DescriptorT> &tree,
                   const openMVG::voctree::Database &db,
                   size_t numResults,
                   std::vector<openMVG::voctree::Matches> &allMatches,
                   std::map<size_t, openMVG::voctree::Document> &documents);

/**
 * @brief Given an non empty database, it queries the database with a set of images
 * and their associated features and returns, for each image, the first \p numResults best
 * matching documents in the database
 * 
 * @param[in] fileFullPath A file containing the path the features to load, it could be a .txt or an OpenMVG .json
 * @param[in] tree The vocabulary tree to be used for feature quantization
 * @param[in] db The built database
 * @param[in] numResults The number of results to retrieve for each image
 * @param[out] allMatches The matches for all the images
 * 
 * @see queryDatabase()
 */
template<class DescriptorT>
void queryDatabase(const std::string &fileFullPath,
                   const openMVG::voctree::VocabularyTree<DescriptorT> &tree,
                   const openMVG::voctree::Database &db,
                   size_t numResults,
                   std::vector<openMVG::voctree::Matches> &allMatches);


} //namespace voctree
} //namespace openMVG

#include "databaseIO.tcc"