/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: KnobOrange.cpp 2070 2014-09-27 23:35:11Z tk $

#include "KnobOrange.h"

//==============================================================================
KnobOrange::KnobOrange(const String &name)
    : Knob(name, 48, 48, ImageCache::getFromMemory(BinaryData::knob_orange_png, BinaryData::knob_orange_pngSize))
{
}

KnobOrange::~KnobOrange()
{
}
