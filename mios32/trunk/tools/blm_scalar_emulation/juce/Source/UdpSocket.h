/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: UdpSocket.h 1078 2010-09-24 20:17:30Z tk $
/*
 * UDP Socket
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _UDP_SOCKET_H
#define _UDP_SOCKET_H

#include "JuceHeader.h"
#include "UdpSocket.h"


class UdpSocket
{
public:
    //==============================================================================
    UdpSocket(void);
    ~UdpSocket();

    //==============================================================================

    bool connect(const String& remoteHost, const unsigned& _portNumberRead, const unsigned& _portNumberWrite);
    void disconnect(void);
    unsigned write(unsigned char *datagram, unsigned len);
    unsigned read(unsigned char *datagram, unsigned maxLen);


protected:

    int oscServerSocket;
    long remoteAddress;
    unsigned portNumberWrite;
};

#endif /* _UDP_SOCKET_H */
