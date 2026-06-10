/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2026 Tencent. All rights reserved.
//
//  Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
//  in compliance with the License. You may obtain a copy of the License at
//
//      https://opensource.org/licenses/BSD-3-Clause
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <optional>
#include "tgfx/core/Image.h"
#include "tgfx/core/Point.h"
#include "tgfx/core/RRect.h"
#include "tgfx/layers/StrokeAlign.h"
#include "tgfx/layers/layerstyles/LayerStyleDrawSource.h"
#include "tgfx/layers/layerstyles/StyledShape.h"

namespace tgfx {

/**
 * Builds a fill StyledShape whose shape is the axis-aligned bounding rect of the supplied path.
 */
std::optional<StyledShape> MakeBoundsStyledShape(const Path& path);

/**
 * Returns true when the path can be represented as a Rect, Oval or RRect — the three shape types
 * for which spread is computed analytically (outset / inset of bounds + corner radii) rather than
 * via a path stroker. When this returns false, shadow LayerStyles fall back to using the content
 * image without spread.
 */
bool CanSpreadAsRRect(const Path& path);

/**
 * Returns a path suitable for analytic spread. If the path is a Rect, Oval or RRect, returns it
 * as-is. Otherwise returns the axis-aligned bounding rect of the path. Returns an empty path when
 * the input is empty.
 */
Path GetSpreadableFillPath(const Path& path);

/**
 * Outsets the supplied path by `distance` analytically. The path must be a Rect, Oval or RRect
 * (see CanSpreadAsRRect); other shapes return an empty path. Rect outset preserves sharp corners,
 * Oval outset preserves the oval shape, and RRect outset grows every corner radius by `distance`.
 * `distance` is assumed to be >= 0.
 */
Path MakeOutsetShape(const Path& path, float distance);

/**
 * Insets the supplied path by `distance` analytically. The path must be a Rect, Oval or RRect.
 * Returns an empty path when the inset collapses the bounds to zero or negative width / height,
 * or when corner radii would underflow past zero. `distance` is assumed to be >= 0.
 */
Path MakeInsetShape(const Path& path, float distance);

/**
 * Outsets the supplied RRect by `distance`: the bounds expand by `distance` on every side and
 * each corner radius grows by `distance`. `distance` is assumed to be >= 0.
 */
RRect MakeOutsetRRect(const RRect& rRect, float distance);

/**
 * Insets the supplied RRect by `distance`: the bounds shrink by `distance` on every side and
 * each corner radius shrinks by the same amount (clamped to 0). Returns nullopt when the inset
 * collapses the bounds to zero or negative width / height. `distance` is assumed to be >= 0.
 */
std::optional<RRect> MakeInsetRRect(const RRect& rRect, float distance);

/**
 * Builds a StyledShape from the given shape and fill/stroke information. When hasFill is true, the
 * result is a fill shape with fillOutset derived from the stroke (if hasStroke is also true).
 * When hasFill is false, the result is a stroke shape with the given width and alignment.
 */
StyledShape MakeStyledShape(std::shared_ptr<Shape> shape, bool hasFill, bool hasStroke,
                            float strokeWidth, StrokeAlign strokeAlign);

/**
 * Holds the rasterized shadow source image and its offset relative to the content image.
 */
struct ShadowSourceImage {
  std::shared_ptr<Image> image = nullptr;
  /**
   * The offset of the shadow source image relative to the content image, scaled by contentScale.
   */
  Point offset = {};
};

/**
 * Produces the alpha source image for shadow filters. When contentShape is available and spread is
 * non-zero, rasterizes the spread-applied geometry into a tightly-sized image. Otherwise returns
 * source.content unchanged. Positive spread outsets the shape, negative spread insets it.
 */
ShadowSourceImage MakeShadowSourceImage(const LayerStyleDrawSource& source, float spread);

}  // namespace tgfx
