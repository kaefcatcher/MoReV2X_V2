/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Marco Miozzo <mmiozzo@cttc.es>
 */

#include <ns3/waveform-generator.h>
#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cmath>
#include <ns3/simulator.h>
#include "ns3/spectrum-error-model.h"
#include "nist-lte-phy.h"
#include "nist-lte-net-device.h"
#include <ns3/double.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NistLtePhy");

NS_OBJECT_ENSURE_REGISTERED (NistLtePhy);


NistLtePhy::NistLtePhy ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("This constructor should not be called");
}

NistLtePhy::NistLtePhy (Ptr<NrV2XSpectrumPhy> dlPhy, Ptr<NrV2XSpectrumPhy> ulPhy)
  : m_downlinkSpectrumPhy (dlPhy),
    m_uplinkSpectrumPhy (ulPhy),
//    m_tti (0.001),
//    m_tti (0.0005),
    m_ulBandwidth (0),
    m_dlBandwidth (0),
    m_rbgSize (0),
    m_macChTtiDelay (0),
    m_cellId (0)
{
  NS_LOG_FUNCTION (this);
}


TypeId
NistLtePhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NistLtePhy")
    .SetParent<Object> ()
    .AddAttribute ("TTI",
                   "PHY layer TTI",
		   DoubleValue (0.001),
		   MakeDoubleAccessor (&NistLtePhy::m_tti),
		   MakeDoubleChecker<double> ())
  ;
  return tid;
}


NistLtePhy::~NistLtePhy ()
{
  NS_LOG_FUNCTION (this);
}

void
NistLtePhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_packetBurstQueue.clear ();
  m_controlMessagesQueue.clear ();
  m_downlinkSpectrumPhy->Dispose ();
  m_downlinkSpectrumPhy = 0;
  m_uplinkSpectrumPhy->Dispose ();
  m_uplinkSpectrumPhy = 0;
  m_netDevice = 0;
  Object::DoDispose ();
}

void
NistLtePhy::SetDevice (Ptr<NistLteNetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_netDevice = d;
}


Ptr<NistLteNetDevice>
NistLtePhy::GetDevice ()
{
  NS_LOG_FUNCTION (this);
  return m_netDevice;
}

Ptr<NrV2XSpectrumPhy> 
NistLtePhy::GetDownlinkSpectrumPhy ()
{
  return m_downlinkSpectrumPhy;
}

Ptr<NrV2XSpectrumPhy> 
NistLtePhy::GetUplinkSpectrumPhy ()
{
  return m_uplinkSpectrumPhy;
}


void
NistLtePhy::SetDownlinkChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_downlinkSpectrumPhy->SetChannel (c);
}

void
NistLtePhy::SetUplinkChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_uplinkSpectrumPhy->SetChannel (c);
}

void
NistLtePhy::SetTti (double tti)
{
  NS_LOG_FUNCTION (this << tti);
  m_tti = tti;
}


double
NistLtePhy::GetTti (void) const
{
  NS_LOG_FUNCTION (this << m_tti);
  return m_tti;
}


uint16_t
NistLtePhy::GetSrsPeriodicity (uint16_t srcCi) const
{
  // from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
  uint16_t SrsPeriodicity[9] = {0, 2, 5, 10, 20, 40, 80, 160, 320};
  uint16_t SrsCiLow[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiHigh[9] = {0, 1, 6, 16, 36, 76, 156, 316, 636};
  uint8_t i;
  for (i = 8; i > 0; i --)
    {
      if ((srcCi>=SrsCiLow[i])&&(srcCi<=SrsCiHigh[i]))
        {
          break;
        }
    }
  return SrsPeriodicity[i];
}

uint16_t
NistLtePhy::GetSrsSubframeOffset (uint16_t srcCi) const
{
  // from 3GPP TS 36.213 table 8.2-1 UE Specific SRS Periodicity
  uint16_t SrsSubframeOffset[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiLow[9] = {0, 0, 2, 7, 17, 37, 77, 157, 317};
  uint16_t SrsCiHigh[9] = {0, 1, 6, 16, 36, 76, 156, 316, 636};
  uint8_t i;
  for (i = 8; i > 0; i --)
    {
      if ((srcCi>=SrsCiLow[i])&&(srcCi<=SrsCiHigh[i]))
        {
          break;
        }
    }
  return (srcCi - SrsSubframeOffset[i]);
}

uint8_t
NistLtePhy::GetRbgSize (void) const
{
  return m_rbgSize;
}

void
NistLtePhy::SetMacPdu (Ptr<Packet> p)
{
  m_packetBurstQueue.at (m_packetBurstQueue.size () - 1)->AddPacket (p);
  NS_LOG_UNCOND("NistLtePhy::SetMacPdu (Ptr<Packet> p) - added new packet to the burst, packetSize: " << p->GetSize());
}

Ptr<PacketBurst>
NistLtePhy::GetPacketBurst (void)
{
  if (m_packetBurstQueue.at (0)->GetSize () > 0)
    {
      Ptr<PacketBurst> ret = m_packetBurstQueue.at (0)->Copy ();
      m_packetBurstQueue.erase (m_packetBurstQueue.begin ());
      m_packetBurstQueue.push_back (CreateObject <PacketBurst> ());
      return (ret);
    }
  else
    {
      m_packetBurstQueue.erase (m_packetBurstQueue.begin ());
      m_packetBurstQueue.push_back (CreateObject <PacketBurst> ());
      return (0);
    }
}


void
NistLtePhy::SetControlMessages (Ptr<NistLteControlMessage> m)
{
  // In uplink the queue of control messages and packet are of different sizes
  // for avoiding TTI cancellation due to synchronization of subframe triggers
  m_controlMessagesQueue.at (m_controlMessagesQueue.size () - 1).push_back (m);
}

std::list<Ptr<NistLteControlMessage> >
NistLtePhy::GetControlMessages (void)
{
  NS_LOG_FUNCTION (this);
  if (m_controlMessagesQueue.at (0).size () > 0)
    {
      std::list<Ptr<NistLteControlMessage> > ret = m_controlMessagesQueue.at (0);
      m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
      std::list<Ptr<NistLteControlMessage> > newlist;
      m_controlMessagesQueue.push_back (newlist);
      return (ret);
    }
  else
    {
      m_controlMessagesQueue.erase (m_controlMessagesQueue.begin ());
      std::list<Ptr<NistLteControlMessage> > newlist;
      m_controlMessagesQueue.push_back (newlist);
      std::list<Ptr<NistLteControlMessage> > emptylist;
      return (emptylist);
    }
}


void
NistLtePhy::DoSetCellId (uint16_t cellId)
{
  m_cellId = cellId;
  m_downlinkSpectrumPhy->SetCellId (cellId);
  m_uplinkSpectrumPhy->SetCellId (cellId);
}


} // namespace ns3