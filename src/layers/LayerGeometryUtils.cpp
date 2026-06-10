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

#include "layers/LayerGeometryUtils.h"
#include <algorithm>
#include "core/utils/Log.h"
#include "core/utils/MathExtra.h"
#include "layers/LayerStyleSource.h"
#include "tgfx/core/Canvas.h"
#include "tgfx/core/PictureRecorder.h"
#include "tgfx/core/RRect.h"
#include "tgfx/core/Shape.h"
#include "tgfx/core/Stroke.h"

namespace tgfx {

std::optional<StyledShape> MakeBoundsStyledShape(const Path& path) {
  StyledShape geometry = {};
  if (!path.isEmpty()) {
    Path boundsPath = {};
    boundsPath.addRect(path.getBounds());
    geometry.shape = Shape::MakeFrom(boundsPath);
  }
  geometry.style = PaintStyle::Fill;
  return geometry;
}

bool CanSpreadAsRRect(const Path& path) {
  if (path.isEmpty()) {
    return false;
  }
  return path.isRect() || path.isOval() || path.isRRect(nullptr);
}

Path GetSpreadableFillPath(const Path& path) {
  if (path.isEmpty()) {
    return {};
  }
  if (CanSpreadAsRRect(path)) {
    return path;
  }
  Path boundsPath = {};
  boundsPath.addRect(path.getBounds());
  return boundsPath;
}

Path MakeOutsetShape(const Path& path, float distance) {
  Rect rect = {};
  if (path.isOval(&rect)) {
    rect.outset(distance, distance);
    Path result = {};
    result.addOval(rect);
    return result;
  }
  if (path.isRect(&rect)) {
    rect.outset(distance, distance);
    Path result = {};
    result.addRect(rect);
    return result;
  }
  RRect rRect = {};
  if (path.isRRect(&rRect)) {
    Path result = {};
    result.addRRect(MakeOutsetRRect(rRect, distance));
    return result;
  }
  return {};
}

Path MakeInsetShape(const Path& path, float distance) {
  Rect rect = {};
  if (path.isOval(&rect)) {
    rect.outset(-distance, -distance);
    if (rect.width() <= 0.0f || rect.height() <= 0.0f) {
      return {};
    }
    Path result = {};
    result.addOval(rect);
    return result;
  }
  if (path.isRect(&rect)) {
    rect.outset(-distance, -distance);
    if (rect.width() <= 0.0f || rect.height() <= 0.0f) {
      return {};
    }
    Path result = {};
    result.addRect(rect);
    return result;
  }
  RRect rRect = {};
  if (path.isRRect(&rRect)) {
    auto insetRRect = MakeInsetRRect(rRect, distance);
    if (!insetRRect.has_value()) {
      return {};
    }
    Path result = {};
    result.addRRect(*insetRRect);
    return result;
  }
  return {};
}

RRect MakeOutsetRRect(const RRect& rRect, float distance) {
  auto bounds = rRect.rect();
  bounds.outset(distance, distance);
  auto radii = rRect.radii();
  for (auto& corner : radii) {
    corner.x += distance;
    corner.y += distance;
  }
  RRect result = {};
  result.setRectRadii(bounds, radii);
  return result;
}

std::optional<RRect> MakeInsetRRect(const RRect& rRect, float distance) {
  auto bounds = rRect.rect();
  bounds.outset(-distance, -distance);
  if (bounds.width() <= 0.0f || bounds.height() <= 0.0f) {
    return std::nullopt;
  }
  auto radii = rRect.radii();
  for (auto& corner : radii) {
    corner.x = std::max(0.0f, corner.x - distance);
    corner.y = std::max(0.0f, corner.y - distance);
  }
  RRect result = {};
  result.setRectRadii(bounds, radii);
  return result;
}

static inline float StrokeOutset(float strokeWidth, StrokeAlign align) {
  switch (align) {
    case StrokeAlign::Center:
      return strokeWidth * 0.5f;
    case StrokeAlign::Outside:
      return strokeWidth;
    case StrokeAlign::Inside:
      return 0.0f;
  }
  return 0.0f;
}

StyledShape MakeStyledShape(std::shared_ptr<Shape> shape, bool hasFill, bool hasStroke,
                            float strokeWidth, StrokeAlign strokeAlign) {
  StyledShape result = {};
  result.shape = std::move(shape);
  if (hasFill) {
    result.style = PaintStyle::Fill;
    if (hasStroke) {
      result.fillOutset = StrokeOutset(strokeWidth, strokeAlign);
    }
  } else {
    result.style = PaintStyle::Stroke;
    result.strokeWidth = strokeWidth;
    result.strokeAlign = strokeAlign;
  }
  return result;
}

static inline void DrawSpreadRect(Canvas* canvas, const Rect& rect, PaintStyle style,
                                  float strokeWidth, float fillOutset, float spread) {
  Paint paint = {};
  paint.setColor(Color::Black());
  paint.setAntiAlias(true);
  if (style == PaintStyle::Fill) {
    auto drawRect = rect;
    drawRect.outset(fillOutset + spread, fillOutset + spread);
    if (drawRect.isEmpty()) {
      return;
    }
    canvas->drawRect(drawRect, paint);
  } else {
    DEBUG_ASSERT(strokeWidth * 0.5f + spread > 0.0f);
    paint.setStyle(PaintStyle::Stroke);
    paint.setStroke(Stroke(strokeWidth + 2.0f * spread));
    canvas->drawRect(rect, paint);
  }
}

static inline void DrawSpreadRRect(Canvas* canvas, const RRect& rRect, PaintStyle style,
                                   float strokeWidth, float fillOutset, float spread) {
  Paint paint = {};
  paint.setColor(Color::Black());
  paint.setAntiAlias(true);
  if (style == PaintStyle::Fill) {
    auto outset = fillOutset + spread;
    if (outset > 0) {
      canvas->drawRRect(MakeOutsetRRect(rRect, outset), paint);
    } else if (outset < 0) {
      auto inset = MakeInsetRRect(rRect, -outset);
      if (!inset.has_value()) {
        return;
      }
      canvas->drawRRect(*inset, paint);
    } else {
      canvas->drawRRect(rRect, paint);
    }
  } else {
    DEBUG_ASSERT(strokeWidth * 0.5f + spread > 0.0f);
    paint.setStyle(PaintStyle::Stroke);
    paint.setStroke(Stroke(strokeWidth + 2.0f * spread));
    canvas->drawRRect(rRect, paint);
  }
}

ShadowSourceImage MakeShadowSourceImage(const LayerStyleDrawSource& source, float spread) {
  if (!source.contentShape.has_value() || FloatNearlyZero(spread)) {
    return {source.content, {}};
  }
  auto& styledShape = *source.contentShape;
  DEBUG_ASSERT(styledShape.shape != nullptr);
  // Stroke fully collapsed by negative spread or empty path.
  if ((styledShape.style == PaintStyle::Stroke && styledShape.strokeWidth * 0.5f + spread <= 0.0f) ||
      styledShape.shape == nullptr || styledShape.shape->getPath().isEmpty()) {
    return {nullptr, {}};
  }

  auto path = styledShape.shape->getPath();
  PictureRecorder recorder;
  auto* recordCanvas = recorder.beginRecording();
  recordCanvas->scale(source.contentScale, source.contentScale);
  Rect rect = {};
  RRect rRect = {};
  auto style = styledShape.style;
  auto strokeWidth = styledShape.strokeWidth;
  auto fillOutset = styledShape.fillOutset;
  if (path.isOval(&rect)) {
    DrawSpreadRRect(recordCanvas, RRect::MakeOval(rect), style, strokeWidth, fillOutset, spread);
  } else if (path.isRRect(&rRect)) {
    DrawSpreadRRect(recordCanvas, rRect, style, strokeWidth, fillOutset, spread);
  } else {
    if (!path.isRect(&rect)) {
      // Complex paths use their bounding rect as a fill approximation for the shadow source.
      rect = path.getBounds();
      style = PaintStyle::Fill;
    }
    DrawSpreadRect(recordCanvas, rect, style, strokeWidth, fillOutset, spread);
  }

  auto picture = recorder.finishRecordingAsPicture();
  Point offset = {};
  auto image = ToImageWithOffset(std::move(picture), &offset);
  if (image == nullptr) {
    DEBUG_ASSERT(false);
    return {nullptr, {}};
  }
  return {std::move(image),
          {offset.x - source.contentOffset.x * source.contentScale,
           offset.y - source.contentOffset.y * source.contentScale}};
}

}  // namespace tgfx
