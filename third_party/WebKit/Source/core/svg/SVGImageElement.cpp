/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/svg/SVGImageElement.h"

#include "core/CSSPropertyNames.h"
#include "core/dom/StyleChangeReason.h"
#include "core/frame/ImageBitmap.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/layout/LayoutImageResource.h"
#include "core/layout/svg/LayoutSVGImage.h"

namespace blink {

inline SVGImageElement::SVGImageElement(Document& document)
    : SVGGraphicsElement(SVGNames::imageTag, document),
      SVGURIReference(this),
      x_(SVGAnimatedLength::Create(this,
                                   SVGNames::xAttr,
                                   SVGLength::Create(SVGLengthMode::kWidth),
                                   CSSPropertyX)),
      y_(SVGAnimatedLength::Create(this,
                                   SVGNames::yAttr,
                                   SVGLength::Create(SVGLengthMode::kHeight),
                                   CSSPropertyY)),
      width_(SVGAnimatedLength::Create(this,
                                       SVGNames::widthAttr,
                                       SVGLength::Create(SVGLengthMode::kWidth),
                                       CSSPropertyWidth)),
      height_(
          SVGAnimatedLength::Create(this,
                                    SVGNames::heightAttr,
                                    SVGLength::Create(SVGLengthMode::kHeight),
                                    CSSPropertyHeight)),
      preserve_aspect_ratio_(SVGAnimatedPreserveAspectRatio::Create(
          this,
          SVGNames::preserveAspectRatioAttr)),
      image_loader_(SVGImageLoader::Create(this)),
      needs_loader_uri_update_(true) {
  AddToPropertyMap(x_);
  AddToPropertyMap(y_);
  AddToPropertyMap(width_);
  AddToPropertyMap(height_);
  AddToPropertyMap(preserve_aspect_ratio_);
}

DEFINE_NODE_FACTORY(SVGImageElement)

DEFINE_TRACE(SVGImageElement) {
  visitor->Trace(x_);
  visitor->Trace(y_);
  visitor->Trace(width_);
  visitor->Trace(height_);
  visitor->Trace(preserve_aspect_ratio_);
  visitor->Trace(image_loader_);
  SVGGraphicsElement::Trace(visitor);
  SVGURIReference::Trace(visitor);
}

bool SVGImageElement::CurrentFrameHasSingleSecurityOrigin() const {
  if (LayoutSVGImage* layout_svg_image = ToLayoutSVGImage(GetLayoutObject())) {
    if (layout_svg_image->ImageResource()->HasImage()) {
      if (Image* image =
              layout_svg_image->ImageResource()->CachedImage()->GetImage())
        return image->CurrentFrameHasSingleSecurityOrigin();
    }
  }

  return true;
}

void SVGImageElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableStylePropertySet* style) {
  SVGAnimatedPropertyBase* property = PropertyFromAttribute(name);
  if (property == width_) {
    AddPropertyToPresentationAttributeStyle(style, property->CssPropertyId(),
                                            width_->CssValue());
  } else if (property == height_) {
    AddPropertyToPresentationAttributeStyle(style, property->CssPropertyId(),
                                            height_->CssValue());
  } else if (property == x_) {
    AddPropertyToPresentationAttributeStyle(style, property->CssPropertyId(),
                                            x_->CssValue());
  } else if (property == y_) {
    AddPropertyToPresentationAttributeStyle(style, property->CssPropertyId(),
                                            y_->CssValue());
  } else {
    SVGGraphicsElement::CollectStyleForPresentationAttribute(name, value,
                                                             style);
  }
}

void SVGImageElement::SvgAttributeChanged(const QualifiedName& attr_name) {
  bool is_length_attribute =
      attr_name == SVGNames::xAttr || attr_name == SVGNames::yAttr ||
      attr_name == SVGNames::widthAttr || attr_name == SVGNames::heightAttr;

  if (is_length_attribute || attr_name == SVGNames::preserveAspectRatioAttr) {
    SVGElement::InvalidationGuard invalidation_guard(this);

    if (is_length_attribute) {
      InvalidateSVGPresentationAttributeStyle();
      SetNeedsStyleRecalc(
          kLocalStyleChange,
          StyleChangeReasonForTracing::FromAttribute(attr_name));
      UpdateRelativeLengthsInformation();
    }

    LayoutObject* object = this->GetLayoutObject();
    if (!object)
      return;

    // FIXME: if isLengthAttribute then we should avoid this call if the
    // viewport didn't change, however since we don't have the computed
    // style yet we can't use updateBoundingBox/updateImageContainerSize.
    // See http://crbug.com/466200.
    MarkForLayoutAndParentResourceInvalidation(object);
    return;
  }

  if (SVGURIReference::IsKnownAttribute(attr_name)) {
    SVGElement::InvalidationGuard invalidation_guard(this);
    if (isConnected())
      GetImageLoader().UpdateFromElement(
          ImageLoader::kUpdateIgnorePreviousError);
    else
      needs_loader_uri_update_ = true;
    return;
  }

  SVGGraphicsElement::SvgAttributeChanged(attr_name);
}

bool SVGImageElement::SelfHasRelativeLengths() const {
  return x_->CurrentValue()->IsRelative() || y_->CurrentValue()->IsRelative() ||
         width_->CurrentValue()->IsRelative() ||
         height_->CurrentValue()->IsRelative();
}

LayoutObject* SVGImageElement::CreateLayoutObject(const ComputedStyle&) {
  return new LayoutSVGImage(this);
}

bool SVGImageElement::HaveLoadedRequiredResources() {
  return !needs_loader_uri_update_ && !GetImageLoader().HasPendingActivity();
}

void SVGImageElement::AttachLayoutTree(const AttachContext& context) {
  SVGGraphicsElement::AttachLayoutTree(context);

  if (LayoutSVGImage* image_obj = ToLayoutSVGImage(GetLayoutObject())) {
    if (image_obj->ImageResource()->HasImage())
      return;

    image_obj->ImageResource()->SetImageResource(GetImageLoader().GetImage());
  }
}

Node::InsertionNotificationRequest SVGImageElement::InsertedInto(
    ContainerNode* root_parent) {
  SVGGraphicsElement::InsertedInto(root_parent);
  if (!root_parent->isConnected())
    return kInsertionDone;

  // We can only resolve base URIs properly after tree insertion - hence, URI
  // mutations while detached are deferred until this point.
  if (needs_loader_uri_update_) {
    GetImageLoader().UpdateFromElement(ImageLoader::kUpdateIgnorePreviousError);
    needs_loader_uri_update_ = false;
  } else {
    // A previous loader update may have failed to actually fetch the image if
    // the document was inactive. In that case, force a re-update (but don't
    // clear previous errors).
    if (!GetImageLoader().GetImage())
      GetImageLoader().UpdateFromElement();
  }

  return kInsertionDone;
}

FloatSize SVGImageElement::SourceDefaultObjectSize() {
  if (GetLayoutObject())
    return ToLayoutSVGImage(GetLayoutObject())->ObjectBoundingBox().Size();
  SVGLengthContext length_context(this);
  return FloatSize(width_->CurrentValue()->Value(length_context),
                   height_->CurrentValue()->Value(length_context));
}

const AtomicString SVGImageElement::ImageSourceURL() const {
  return AtomicString(HrefString());
}

void SVGImageElement::DidMoveToNewDocument(Document& old_document) {
  GetImageLoader().ElementDidMoveToNewDocument();
  SVGGraphicsElement::DidMoveToNewDocument(old_document);
}

}  // namespace blink
