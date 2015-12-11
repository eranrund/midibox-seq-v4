/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: ControlGroup.cpp 2070 2014-09-27 23:35:11Z tk $
/*
 * Prototype of a Control Group
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "ControlGroup.h"

//==============================================================================
ControlGroup::ControlGroup(const String &name, const Colour& _headerTextColour, const Colour& _headerBackgroundColour)
    : Component(name)
{
    addAndMakeVisible(headerLabel = new Label("Header", name));
    headerLabel->setJustificationType(Justification::verticallyCentred | Justification::horizontallyCentred);
    headerLabel->setColour(Label::textColourId, _headerTextColour);
    headerLabel->setColour(Label::backgroundColourId, _headerBackgroundColour);

    setSize(100, 50);
}

ControlGroup::~ControlGroup()
{
}

//==============================================================================
void ControlGroup::paint(Graphics& g)
{
    g.fillAll(Colour::greyLevel(0.375f));
}

void ControlGroup::resized()
{
    headerLabel->setBounds(0, 0, getWidth(), 18);
}
