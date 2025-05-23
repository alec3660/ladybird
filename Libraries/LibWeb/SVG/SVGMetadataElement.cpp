/*
 * Copyright (c) 2024, Jamie Mansfield <jmansfield@cadixdev.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Bindings/SVGMetadataElementPrototype.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/Page/Page.h>
#include <LibWeb/SVG/SVGMetadataElement.h>

namespace Web::SVG {

GC_DEFINE_ALLOCATOR(SVGMetadataElement);

SVGMetadataElement::SVGMetadataElement(DOM::Document& document, DOM::QualifiedName qualified_name)
    : SVGElement(document, move(qualified_name))
{
}

void SVGMetadataElement::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(SVGMetadataElement);
    Base::initialize(realm);
}

GC::Ptr<Layout::Node> SVGMetadataElement::create_layout_node(GC::Ref<CSS::ComputedProperties>)
{
    return nullptr;
}

}
