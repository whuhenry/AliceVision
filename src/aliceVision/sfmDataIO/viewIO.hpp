// This file is part of the AliceVision project.
// Copyright (c) 2017 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/sfmData/View.hpp>
#include <aliceVision/camera/cameraCommon.hpp>
#include <aliceVision/camera/IntrinsicBase.hpp>

#include <boost/filesystem.hpp>

#include <memory>

namespace aliceVision {
namespace sfmDataIO {

enum class EViewIdMethod
{
    METADATA,
    FILENAME
};

inline std::string EViewIdMethod_enumToString(EViewIdMethod viewIdMethod)
{
    switch(viewIdMethod)
    {
        case EViewIdMethod::METADATA: return "metadata";
        case EViewIdMethod::FILENAME: return "filename";
    }
    throw std::out_of_range("Invalid ViewIdMethod type Enum: " + std::to_string(int(viewIdMethod)));
}

inline EViewIdMethod EViewIdMethod_stringToEnum(const std::string& viewIdMethod)
{
    if(viewIdMethod == "metadata") return EViewIdMethod::METADATA;
    if(viewIdMethod == "filename") return EViewIdMethod::FILENAME;

    throw std::out_of_range("Invalid ViewIdMethod type string " + viewIdMethod);
}

inline std::ostream& operator<<(std::ostream& os, EViewIdMethod s)
{
    return os << EViewIdMethod_enumToString(s);
}

inline std::istream& operator>>(std::istream& in, EViewIdMethod& s)
{
    std::string token;
    in >> token;
    s = EViewIdMethod_stringToEnum(token);
    return in;
}

/**
 * @brief update an incomplete view (at least only the image path)
 * @param view The given incomplete view
 * @param[in] viewIdMethod ViewId generation method to use
 * @param[in] viewIdRegex Optional regex used when viewIdMethod is FILENAME
 */
void updateIncompleteView(sfmData::View& view, EViewIdMethod viewIdMethod = EViewIdMethod::METADATA, const std::string& viewIdRegex = "");

/**
 * @brief create an intrinsic for the given View
 * @param[in] view The given view
 * @param[in] mmFocalLength (-1 if unknown)
 * @param[in] sensorWidth (-1 if unknown)
 * @param[in] defaultFocalLengthPx (-1 if unknown)
 * @param[in] defaultFieldOfView (-1 if unknown)
 * @param[in] defaultIntrinsicType (unknown by default)
 * @param[in] defaultPPx (-1 if unknown)
 * @param[in] defaultPPy (-1 if unknown)
 * @param[in] allowedEintrinsics The intrinsics values that can be attributed
 * @return shared_ptr IntrinsicBase
 */
std::shared_ptr<camera::IntrinsicBase> getViewIntrinsic(
					const sfmData::View& view, double mmFocalLength = -1.0, double sensorWidth = -1,
					double defaultFocalLengthPx = -1, double defaultFieldOfView = -1,
					camera::EINTRINSIC defaultIntrinsicType = camera::EINTRINSIC::UNKNOWN,
					camera::EINTRINSIC allowedEintrinsics = camera::EINTRINSIC::VALID_CAMERA_MODEL,
					double defaultPPx = -1, double defaultPPy = -1);

/**
* @brief Allows you to retrieve the files paths corresponding to a view by searching through a list of folders.
*        Filename must be the same or equal to the viewId.
* @param[in] the view
* @param[in] the folder list
* @return the list of paths to the corresponding view if found in the folders, otherwise returns an empty list.
*/
std::vector<std::string> viewPathsFromFolders(const sfmData::View& view, const std::vector<std::string>& folders);

} // namespace sfmDataIO
} // namespace aliceVision
