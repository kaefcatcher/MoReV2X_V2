/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
  */

#include <ns3/applications-module.h>
#include <ns3/config-store.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/nist-lte-helper.h>
#include <ns3/MoReV2X-module.h>
#include "ns3/nist-sl-resource-pool-factory.h"
#include "ns3/nist-sl-preconfig-pool-factory.h"
#include "ns3/delay-jitter-estimation.h"
#include "ns3/packet.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include "ns3/packet-tag-list.h"
#include <ns3/multi-model-spectrum-channel.h>
#include "ns3/random-variable-stream.h"
#include <fstream>
#include <iostream>
#include "ns3/buildings-helper.h"
#include "ns3/nr-v2x-propagation-loss-model.h"
#include <cmath>
#include "ns3/rng-seed-manager.h"
#include "ns3/nr-v2x-tag.h"
#include "ns3/nr-v2x-utils.h"
#include <random>
#include <ns3/nr-v2x-amc.h>

NS_LOG_COMPONENT_DEFINE ("DebugScript");

using namespace ns3;

uint64_t packetID = 0;
double simTime;

std::default_random_engine generator;
std::discrete_distribution<int> distribution {8, 2};

Ipv4Address groupAddress; //use multicast address as destination --> broadcast 

int TrepPrint = 100; // (ms). Set the interval for printing and updating the V-UE stats
//int PDBaperiodic = 50;
int Tgen_aperiodic_c;

std::vector<uint16_t> AperiodicPKTs_Size, PeriodicPKTs_Size;
uint16_t LargestAperiodicSize, LargestPeriodicSize, LargestCAMSize;

double MeasInterval;

Ptr<ExponentialRandomVariable> RndExp;
Ptr<ExponentialRandomVariable> RndExp_1;
int ModuloSplit;

bool ExponentialModel;

std::map< uint32_t, std::vector< std::pair<int,int> > > CAMtraces;

std::vector<double> Periodic_Tgen;
std::vector<double> Aperiodic_Tgen_c;
std::vector<double> PDB_Periodic;
std::vector<double> PDB_Aperiodic;

std::vector<double> PrevX, PrevY, PrevZ, VelX, VelY, VelZ; 
std::vector<bool> EnableTX;
std::vector<uint8_t> VehicleTrafficType;

std::vector<uint16_t> Pattern_index;

bool ETSITraffic, avgRRI;

void LoadCAMtraces (NodeContainer VehicleUEs);

void Print (NodeContainer VehicleUEs);

PosEnabler PositionChecker;

bool UrbanScenario;

std::string FilePath;

bool enableUDPfiles;

std::ifstream traceFile;

void opencsv(std::string name) {
  //std::system(("cd xml2csv && python xml2csv.py -p " + name+".xml").c_str());
  traceFile = std::ifstream("xml2csv/" + name + ".csv");
  if (!traceFile.is_open()) {
    std::cerr << "Error opening trace file!" << std::endl;
    exit(1);
  }
}

std::pair<Vector,Vector> getNextCoords(std::ifstream &traceFile) {
  // Define static variable to keep track of file stream and current position
  static std::string currentLine;

  // Read the next line from the file
  if (std::getline(traceFile, currentLine)) {
    // Parse the line to extract x, y, and z positions
    std::istringstream iss(currentLine);
    std::string xposStr, yposStr, zposStr, speedStr;
    std::getline(iss, xposStr, ',');
    std::getline(iss, yposStr, ',');
    std::getline(iss, zposStr, ',');
    std::getline(iss, speedStr, ',');

    // Convert positions to double
    double xpos = std::stod(xposStr);
    double ypos = std::stod(yposStr);
    double zpos = std::stod(zposStr);
    double speed = std::stod(speedStr);

    // Return the Vector with extracted positions and speed
    return std::make_pair(Vector(xpos, ypos, zpos),Vector(speed,0.0,0.0));
  } else {
    // If no more lines in the file, return an empty vector
    return std::make_pair(Vector(0.0, 0.0, 0.0),Vector(0.0,0.0,0.0));
  }
}

void UpdateVehiclePositionsFromTrace(std::ifstream *traceFile, NodeContainer& vehicles) {
    // Loop through all vehicles in the NodeContainer
    for (NodeContainer::Iterator it = vehicles.Begin(); it != vehicles.End(); ++it) {
            Ptr<Node> node = *it;
            Ptr<ConstantVelocityMobilityModel> velMob = node->GetObject<ConstantVelocityMobilityModel>();
            if (!velMob) {
                NS_LOG_WARN("Node does not have ConstantVelocityMobilityModel. Skipping.");
                continue;
            }

            // Update position and velocity of the vehicle
            std::pair<Vector,Vector> newData = getNextCoords(*traceFile);
            Vector newPos = newData.first;
            Vector newSpeed = newData.second;
            velMob->SetPosition(newPos);
            velMob->SetVelocity(newSpeed); // Modify this if velocity in other directions is needed
    }

    // Schedule the next update 1 second later
    // Simulator::Schedule (MilliSeconds (TrepPrint), &Print, vehicles);
    // Simulator::Schedule(Seconds(1.0), &UpdateVehiclePositionsFromTrace, tracefile, vehicles);
}


uint32_t PacketSizeDistribution(void)
{
  int sample = distribution(generator);

  if (sample == 0)
    return 100;
  else if (sample == 1)
    return 300;
  else 
    NS_FATAL_ERROR("Unhandled option in PMF");

/*  
  int nrolls = 1000;
  std::vector<int> p;
  for (int i=0; i<nrolls; i++)
  {
    int sample = distribution(generator);
    if (sample == 0)
      std::cout << "100 B" << std::endl;
    else if (sample == 1)
      std::cout << "300 B" << std::endl;
    else
     NS_FATAL_ERROR("Unhandled option in PMF");
    p.push_back(sample);
  }

  double avg_100=0, avg_300=0;
  for (std::vector<int>::iterator it = p.begin(); it!=p.end(); it++)
   if(*it==0)
     avg_100++;
   else 
     avg_300++;

  std::cout << p.size() << "," << avg_100/p.size() << "," << avg_300/p.size() << std::endl;

  return packetSize;*/
}

void
UdpClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();
  uint32_t ReservationSize;
  double numHops = 0;  
  double T_gen = 0;
  bool insideTX;

  Ptr<UniformRandomVariable> packetSize_index = CreateObject<UniformRandomVariable>();

  if (EnableTX[nodeId-1]) //Not all nodes are enabled to transmit (see SUMO simulation details)
  {
    SeqTsHeader seqTs;  // Packet header for UDP Client/Server application
    seqTs.SetSeq (m_sent); // The packet header must be sequentially increased. Use the packet number m_sent

    /* SET THE PACKET TAG
    and CONFIGURE UdpClient::Send */

    NrV2XTag v2xTag;
    v2xTag.SetGenTime (Simulator::Now ().GetSeconds ());
    v2xTag.SetMessageType (0x00); //CAM, @LUCA just want to work with CAMs
    v2xTag.SetTrafficType (VehicleTrafficType[nodeId-1]); // Coexistence of periodic and aperiodic traffic
    v2xTag.SetPPPP (0x00); // PPPP for CAM
    v2xTag.SetPrsvp ((double) 100); // the required PHY reservation interval. For periodic traffic is ok
    v2xTag.SetNodeId ((uint32_t) nodeId); // Encapsulate the nodeId. It will be used at MAC layer
    v2xTag.SetReselectionCounter((uint16_t)10000); //safe value for standard-compliant Cresel assignment

    if (VehicleTrafficType[nodeId-1] == 0x01) // Aperiodic traffic
    {
      if (ETSITraffic) 
      {
        T_gen = CAMtraces[nodeId][Pattern_index[nodeId-1]].first;

        v2xTag.SetPdb ((double)100); // @LUCA modified later 
        m_size = CAMtraces[nodeId][Pattern_index[nodeId-1]].second;
        ReservationSize = LargestCAMSize;
        NS_LOG_UNCOND("Udp node " << nodeId << ": transmitting packet with size: " << m_size << " and reserving resources using " << ReservationSize);
        v2xTag.SetPrsvp ((double)100); // the required PHY reservation interval 
        v2xTag.SetReservationSize((uint16_t) ReservationSize + 34);

      }
      else //Exponential model
      {
        //if (nodeId % ModuloSplit != 0)
        if (nodeId % ModuloSplit <= 1)
        { 
          T_gen = Aperiodic_Tgen_c[nodeId-1] + RndExp->GetValue (); 
          NS_LOG_INFO("Aperiodic inter-arrival time constant = " << Aperiodic_Tgen_c[nodeId-1] << ", Exp mean = " << RndExp->GetMean()); 
        }
        else
        {
          T_gen = Aperiodic_Tgen_c[nodeId-1] + RndExp_1->GetValue (); 
          NS_LOG_INFO("Aperiodic inter-arrival time constant = " << Aperiodic_Tgen_c[nodeId-1] << ", Exp mean = " << RndExp_1->GetMean());
        }
//        v2xTag.SetPdb ((double)Aperiodic_Tgen_c[nodeId-1]); // @LUCA modified later
        v2xTag.SetPdb ((double)PDB_Aperiodic[nodeId-1]); // @LUCA modified later

        m_size = AperiodicPKTs_Size[packetSize_index->GetInteger(0,AperiodicPKTs_Size.size()-1)];
        //m_size = PacketSizeDistribution();
      //  ReservationSize = LargestAperiodicSize;
        ReservationSize = m_size;

        if (avgRRI)
          v2xTag.SetPrsvp ((double) Aperiodic_Tgen_c[nodeId-1]*2); // average RRI 
        else
          v2xTag.SetPrsvp ((double) Aperiodic_Tgen_c[nodeId-1]); // minimum RRI

        v2xTag.SetReservationSize((uint16_t) ReservationSize + 35);
        NS_LOG_UNCOND("Udp: UE " << nodeId << " transmitting packet with size: " << m_size+35 << " B and reserving resources using " << ReservationSize+35 << "B. RRI " << v2xTag.GetPrsvp() << " ms. Next packet in " << T_gen << " ms");
      }
     // std::cin.get();
      v2xTag.SetPacketSize((uint16_t) m_size + 35);
    }
    else  // Periodic traffic
    {
      T_gen = 100;
      v2xTag.SetPrsvp ((double) 100);
//      v2xTag.SetPdb ((double) v2xTag.GetPrsvp ()); // @LUCA modified later
      v2xTag.SetPdb ((double)PDB_Periodic[nodeId-1]); // @LUCA modified later
      m_size = PeriodicPKTs_Size[Pattern_index[nodeId-1]%5];
//      m_size = AperiodicPKTs_Size[packetSize_index->GetInteger(0,AperiodicPKTs_Size.size()-1)];

   //   ReservationSize = LargestPeriodicSize;
      ReservationSize = m_size;
      v2xTag.SetPacketSize((uint16_t) m_size + 35);
      v2xTag.SetReservationSize((uint16_t) ReservationSize + 35);
      NS_LOG_UNCOND("Udp: UE " << nodeId << " transmitting packet with size: " << m_size+35 << " B and reserving resources using " << ReservationSize+35 << "B. RRI " << v2xTag.GetPrsvp() << " ms. Next packet in " << T_gen << " ms");
    }
    packetID = packetID + 1; // increment the packet ID number 
    v2xTag.SetIntValue(packetID); 
    v2xTag.SetPacketId(packetID); 
    v2xTag.SetDoubleValue(Simulator::Now().GetSeconds());
    Ptr<MobilityModel> mobility = GetNode()->GetObject<MobilityModel>();
    Vector currentPos = mobility -> GetPosition();
    double xPosition = currentPos.x;
    double yPosition = currentPos.y;
    // Set generation position in tag
    v2xTag.SetGenPosX(xPosition);
    v2xTag.SetGenPosY(yPosition);  
    v2xTag.SetNumHops(numHops); // 0 by default

    /*  NOW CREATE THE PACKET */
    Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header

    p->AddHeader (seqTs);  // Add the header to the packet

    p->AddByteTag (v2xTag); // Attach the tag
    if ((VehicleTrafficType[nodeId-1] == 0x01) && (ETSITraffic) && enableUDPfiles) 
    {
      std::ofstream CAMdebug;
      CAMdebug.open(FilePath + "CAMdebugFile.txt", std::ios_base::app);
      CAMdebug << packetID << "," << Simulator::Now().GetSeconds() << "," << nodeId << "," <<  Pattern_index[nodeId-1] << "," <<  CAMtraces[nodeId][Pattern_index[nodeId-1]].first << "," <<  CAMtraces[nodeId][Pattern_index[nodeId-1]].second << "\r\n" ;
      CAMdebug.close();
    }
    Point point = {(int)xPosition, (int)yPosition}; 
    insideTX = PositionChecker.isInsidePoly("TX", point);
 //   if ((xPosition >= 1500) && (xPosition <= 3500)){
    if (insideTX && enableUDPfiles){
    std::ofstream filetest;
    filetest.open(FilePath + "TxFile.txt", std::ios_base::app);
    filetest << packetID << "," << Simulator::Now().GetSeconds() << "," << nodeId << "," << xPosition << "," << yPosition << "," << (int)v2xTag.GetMessageType () << "," << (int)v2xTag.GetTrafficType () << "," << m_size+34 << "\r\n" ;
    filetest.close();
    }
    std::stringstream peerAddressStringStream;
    if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
    else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }
    // my custom socket to send data
    TypeId UDPtid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    // This method wraps the creation of sockets that is performed on a given node by a SocketFactory specified by TypeId
    Ptr<Socket> sendSocket = Socket::CreateSocket(currentNode, UDPtid);
    sendSocket -> SetAllowBroadcast(true);  // Configure whether broadcast transmissions are allowed
    Ipv4Address destAddress = groupAddress; // Important! The SlTft will check for the groupAddress and forward the packet on the Sidelink bearer
    //Ipv4Address destAddress ("225.255.255.255");
    uint16_t destPort = 8000;

    if (Ipv4Address::IsMatchingType(destAddress) == true)
    {
      sendSocket->Bind ();
      sendSocket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(destAddress), destPort));
          //sendSocket->ShutdownRecv();
    }
    else if (Ipv6Address::IsMatchingType(destAddress) == true)
    {
      sendSocket->Bind6 ();
      sendSocket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(destAddress), destPort));
    } 
    // Send data on the socket and check if returns an error or not (if -> Send fails it returns -1)
    if ((sendSocket->Send (p)) >= 0)
    {
      ++m_sent;
     // NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to " << peerAddressStringStream.str () << " Uid: " << p->GetUid () << " Time: " << (Simulator::Now ()).GetSeconds ());
      NS_LOG_INFO ("TraceDelay TX " << p->GetSize() << " bytes to " << peerAddressStringStream.str () << " Uid: " << p->GetUid () << " Time: " << (Simulator::Now ()).GetSeconds ());
    }
    else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }
//    std::cin.get();

    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();

    if (m_sent < m_count)
    {
       if (VehicleTrafficType[nodeId-1] == 0x00)  //--- If it's PERIODIC traffic ---
       {
         Pattern_index[nodeId-1]++;
//         m_sendEvent = Simulator::Schedule (m_interval, &UdpClient::Send, this); 
         m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 
       }
       else //--- If it's APERIODIC traffic ---
       {  
         if (ETSITraffic)
         {
           Pattern_index[nodeId-1]++;
           m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 

         }
         else
         {
          // NS_LOG_UNCOND("Node ID " << nodeId << " Tgen " << T_gen);
           // std::cin.get();
           m_sendEvent = Simulator::Schedule (MilliSeconds(T_gen), &UdpClient::Send, this); 
         }
       }

    }
  } // end if Enable
  else
  {
    m_sendEvent = Simulator::Schedule (MilliSeconds(TrepPrint), &UdpClient::Send, this); 
  }

}


void PacketSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from,localAddress;
  Ptr<Node> currentNode = GetNode();
  uint32_t nodeId = currentNode -> GetId();

  double tGenSec;
  double genPosX;
  double genPosY;
  double rxPosX;
  double rxPosY;
  double TxRxDistance = 0;
  uint64_t rxPacketID = 0;
  uint32_t numHops;
  uint32_t lastReceivedPacketId = 0;
//  bool rebroadcast = true;
  uint8_t messageType = 0x00;
  uint8_t alreadyReceived = 0;
  bool insideRX;

  NrV2XTag rxV2xTag;

  std::ofstream filetest;
  std::ofstream fileTx;
  while ((packet = socket->RecvFrom (from)))
  {
    if (packet->GetSize () == 0)
    { 
      break;
    }
    m_totalRx += packet->GetSize ();
    if(packet->FindFirstMatchingByteTag(rxV2xTag)) // If the packet tag exists, inspect its content
    {
      rxPacketID = rxV2xTag.GetIntValue();
      messageType = rxV2xTag.GetMessageType ();       
      // Retrieve node state
      Ptr<LTENodeState> nodeState = Create<LTENodeState> ();
      nodeState = currentNode -> GetObject<LTENodeState> ();
      lastReceivedPacketId = nodeState -> GetLastRcvPacketId();
      lastReceivedPacketId+=0;
      //Update the last received packet ID
      nodeState -> SetLastRcvPacketId(rxPacketID);

      //NS_LOG_UNCOND("\nOk: " << rxPacketID << ", rebroadcast: " << rebroadcast);
      NS_LOG_UNCOND("\nOk: " << rxPacketID);
      tGenSec = rxV2xTag.GetDoubleValue();
      //TXnodeId = rxV2xTag.GetNodeId();
      genPosX = rxV2xTag.GetGenPosX();
      genPosY = rxV2xTag.GetGenPosY();
      Ptr<MobilityModel> mobility = GetNode()->GetObject<MobilityModel>();
      Vector currentPos = mobility -> GetPosition(); 
      rxPosX = currentPos.x;
      rxPosY = currentPos.y;      
      TxRxDistance = std::sqrt(std::pow(genPosX - rxPosX, 2) + std::pow(genPosY - rxPosY, 2));       
      numHops = rxV2xTag.GetNumHops();  
    }
      
    SeqTsHeader seqTs;
    packet->RemoveHeader (seqTs);
    uint32_t currentSequenceNumber = seqTs.GetSeq ();
    currentSequenceNumber+=0;
    Point p = {(int)rxPosX, (int)rxPosY}; 
    insideRX = PositionChecker.isInsidePoly("RX", p);
    if (insideRX && enableUDPfiles)
    {
      filetest.open(FilePath + "RxFile.txt",std::ios_base::app);
      filetest << rxPacketID << "," << tGenSec << "," << Simulator::Now().GetSeconds() << "," << Simulator::Now().GetSeconds() - tGenSec <<"," << nodeId << "," << packet->GetSize () << "," << TxRxDistance << "," << numHops << "," << (int) messageType << "," << (int)rxV2xTag.GetTrafficType () << "," << (int) alreadyReceived << "\r\n" ;
      filetest.close();
    }
  }
}


void LoadCAMtraces (NodeContainer VehicleUEs)
{
    uint32_t ID;

    for (NodeContainer::Iterator L = VehicleUEs.Begin(); L != VehicleUEs.End(); ++L)
    {
      Ptr<Node> node = *L;
      ID = node->GetId ();     
  //    Pattern_index[ID-1] = 0;
      std::string line,word;
      std::vector<std::string> row; 
      std::vector<int> row_int; 
      std::ifstream CAMtraceFile;
      CAMtraceFile.open("/home/luca/Desktop/C-V2V/src/MoReV2X/CAM-tools/CAM-model/CAMtraces/CAMtrace_" + std::to_string(ID) + ".csv");
      if (CAMtraceFile.is_open())
      {  
        while ( getline (CAMtraceFile,line) )
        {
          row.clear();
          row_int.clear();
          std::stringstream s(line); 
          while (getline(s, word, ',')) 
          { 
             row.push_back(word);
             row_int.push_back(std::stoi(word));
          }
          NS_LOG_UNCOND("Node ID " << ID << " row " << row_int[1]);
   //       CAMtraces[ID].push_back(row_int);
          CAMtraces[ID].push_back(std::make_pair(row_int[1],row_int[2]));
        }
      }
      CAMtraceFile.close();   
    }
//  std::cin.get();
}




void Print (NodeContainer VehicleUEs) {
        uint32_t ID;
        bool inside;
        std::ofstream positFile;  
        positFile.open(FilePath + "posFile.txt",std::ofstream::app);
        for (NodeContainer::Iterator L = VehicleUEs.Begin(); L != VehicleUEs.End(); ++L)
        {
            Ptr<Node> node = *L;
            ID = node->GetId ();
                
            Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
            if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
            Vector pos = mob->GetPosition ();
            Vector vel = mob->GetVelocity ();
            // Vector vel;
            // VelX[ID-1] = 0;
            // VelY[ID-1] = 0;
            // VelZ[ID-1] = 0;

            // VelX[ID-1] = (pos.x-PrevX[ID-1])/MeasInterval;
            // VelY[ID-1] = (pos.y-PrevY[ID-1])/MeasInterval;
            // VelZ[ID-1] = (pos.z-PrevZ[ID-1])/MeasInterval;

       //     vel.x = VelX[ID-1];
       //     vel.y = VelY[ID-1];
       //     vel.z = VelZ[ID-1];

            Point p = {(int)pos.x, (int)pos.y}; 
            inside = PositionChecker.isInsidePoly("RX", p);

            if (inside){
              positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << vel.x << "," << vel.y << "," << vel.z << "," << (int)VehicleTrafficType[ID-1] << "," << "1" << "\r\n";
            }
            else
              positFile << Simulator::Now().GetSeconds() << "," << ID << "," << pos.x << "," << pos.y << "," << pos.z << "," << vel.x << "," << vel.y << "," << vel.z << "," << (int)VehicleTrafficType[ID-1] << "," << "0" << "\r\n";

            EnableTX[ID-1] = true;
            if (!PositionChecker.isEnabled(pos))
              EnableTX[ID-1] = false;


           PrevX[ID-1] = pos.x;
           PrevY[ID-1] = pos.y;
           PrevZ[ID-1] = pos.z;
        }         
        Simulator::Schedule (MilliSeconds(TrepPrint), &Print, VehicleUEs);
        if (traceFile.is_open())  
        Simulator::Schedule(MilliSeconds(TrepPrint), &UpdateVehiclePositionsFromTrace, &traceFile, VehicleUEs);    
        positFile.close();
}



int
main (int argc, char *argv[])
{

//-----DEBUGGING TOOLS------------------------------------------------------------------------
//  LogComponentEnable("DebugScript", LOG_LEVEL_ALL);  
//  LogComponentEnable("NistLteHelper", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteRrcProtocolIdeal", LOG_LEVEL_ALL);
//  LogComponentEnable("BuildingsPropagationLossModel", LOG_LEVEL_ALL);
//  LogComponentEnable("NrV2XPhyErrorModel", LOG_LEVEL_ALL)
//  LogComponentEnable("NrV2XUeMac", LOG_LEVEL_ALL);
//  LogComponentEnable("NrV2XUePhy", LOG_LEVEL_ALL);
//  LogComponentEnable("NrV2XSpectrumPhy", LOG_LEVEL_ALL);
//  LogComponentEnable("NrV2XPropagationLossModel", LOG_LEVEL_ALL);
//  LogComponentEnable("NrV2XAmc", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteRlcUm", LOG_LEVEL_ALL);
//  LogComponentEnable("NistLteUeRrc", LOG_LEVEL_ALL);
//  LogComponentEnable("NrV2XSpectrumValueHelper", LOG_LEVEL_ALL); //TODO
//  LogComponentEnable("NistLteSlInterference", LOG_LEVEL_ALL); 
//  LogComponentEnable("MultiModelSpectrumChannel", LOG_LEVEL_ALL); 
  //LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
//--------------------------------------------------------------------------------------------
  
  // Provides uniform random variables.
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>(); 
 

  // Initialize SUMO integration 
  bool g_sumoOn = true;
  // std::string g_tracefile = "home/kaefcatcher/Uni/MoReV2X_V2/scratch/trace.csv";

  // Initialize some values
  uint32_t mcs = 13; // The Modulation and Coding Scheme
  uint32_t pscchLength = 8;
  std::string period="sf40";
  simTime = 30;
  double ueTxPower = 23.0; // [dBm]
  uint32_t ueCount = 4; // Number of V-UEs 
  bool verbose = true;
  enableUDPfiles = false;
  //Default configuration
  uint16_t OFDM_numerology = 0; //Default value is 0 = 15 KHz SCS
  uint16_t channelBW = 10; //In MHz, default
  uint16_t channelBW_RBs;
  uint32_t subchannelSize = 10; //Default
  uint32_t highwayLength = 5000;

  bool IBE = true;

  std::string outputPath;

  bool DynamicSchedulingMode2 = false;
  bool AdaptiveSchedulingMode2 = false;
  
  bool FrequencyReuse = false;
  uint32_t ReuseDistance = 0;
  double GeoCellSize;

  bool ReTranmissions = false;

  bool ReEvaluation = false; //Default
  bool AllSlots_ReEvaluation = false; //Default
  bool UMH_ReEvaluation = false; //Default

  bool VariablePacketSize = false;

  int inputPDB = 0;

  double RefSensitivity, CBR_RSSIthreshold;
  double MAC_RSRPthreshold = -128.0;
//  AperiodicPKTs_Size = packetSize;  // Can be set with a different dimension too.

  bool PeriodicTraffic = false;
  bool AperiodicTraffic = false;
  bool MixedTraffic = false;
  int PeriodicPercentage = 0;

  ETSITraffic = false;

  avgRRI = false;

  bool CtrlErrorModelEnabled = true; // Enable error model in the PSCCH

  bool randomV2VSelection = false; // If true, transmission resources are randomly selected
 //uint32_t numPackets = 1;

  bool RxCresel = false;

// Change the random run  
  uint32_t seed = 867; // this is the default seed;
  uint32_t runNumber = 1; // this is the default run --> this will be overridden shortly...

  UrbanScenario = false;  // Enable the urban scenario channel models

  std::map <uint16_t, uint16_t> SCS_factor = {{0,1}, {1,2}, {2,4}, {3,8}};

  // Arrays of SUMO coordinates needed for checking the UEs position
  std::vector<BoundingBox> BBs = {{533, 740, 830, 1027}, {1784, 1938, 1602, 1746}, {2544, 2732, 2654, 2775}, 
                    {2561, 2689, 2989, 3080}, {2278, 2389, 3592, 3716}, {1929, 2211, 3750, 3906}, 
                    {1098, 1223, 3456, 3543}, {525, 618, 2931, 2993}, {400, 541, 2171, 2293}};

  Point polygonTX[] = {{975, 1870}, {1540, 1626}, {1965, 2121}, {2556, 3253}, {1798,3597}, {966,2492}}; 
  Point polygonRX[] = {{962, 1861}, {1541, 1614}, {1975, 2114}, {2572, 3258}, {1793,3609}, {953,2491}}; 

  static std::string traceFileName="";

  CommandLine cmd;
  double pkeep = 0.0;
  // SUMO related cmd arguments
  cmd.AddValue("sumo_on", "Enable SUMO integration", g_sumoOn);
  cmd.AddValue("tracefile", "Path to SUMO trace file", traceFileName);

  // Pkeep
  cmd.AddValue("pkeep", "Persistence scheduling coefficient", pkeep);

  cmd.AddValue ("Vehicles", "Number of vehicles", ueCount);
  cmd.AddValue ("period", "Sidelink period", period);
  cmd.AddValue ("pscchLength", "Length of PSCCH.", pscchLength);
  cmd.AddValue ("mcs", "MCS.", mcs);
  cmd.AddValue ("verbose", "Print time progress.", verbose);
//  cmd.AddValue ("trep", "The Application-layer message repetition", trep);
  cmd.AddValue ("ueTxPower", "The UE TX Power", ueTxPower);
//  cmd.AddValue ("packetSize", "The Application-layer Packet Size in Byte", packetSize);
  cmd.AddValue ("simTime", "The simulated time in seconds", simTime);
  cmd.AddValue ("seed", "The random seed", seed);
  cmd.AddValue ("runNo", "The run number", runNumber);
  cmd.AddValue ("randomV2VSelection", "Whether V2V resources are randomly selected in autonomous scheduling mode", randomV2VSelection);
  cmd.AddValue ("useRxCresel", "Mode 2: use the receiver SCI reselection counter", RxCresel); 

  cmd.AddValue ("noIBE", "Disable In-Band Emissions (IBE)", IBE); 
  cmd.AddValue ("SubChannel", "Subchannel Size in RBs", subchannelSize); 
  cmd.AddValue ("ChannelBW", "Channel bandwidth in MHz", channelBW); 

  cmd.AddValue ("VariableSize", "Generate variable size packets", VariablePacketSize); 

  cmd.AddValue ("Dynamic", "Enable Mode 2 Dynamic Scheduling", DynamicSchedulingMode2); 
  cmd.AddValue ("Adaptive", "Enable Mode 2 Adaptive Scheduling (mixed traffic only)", AdaptiveSchedulingMode2); 
  cmd.AddValue ("ReTx", "Allow re-transmissions (blind)", ReTranmissions); 
  cmd.AddValue ("ReEvaluation", "Allow re-evaluation of selected resources", ReEvaluation); 
  cmd.AddValue ("AllSlots", "Perform re-evaluation in all-slots", AllSlots_ReEvaluation); 
  cmd.AddValue ("UMH", "Perform re-evaluation only on re-transmissions", UMH_ReEvaluation); 
  cmd.AddValue ("RSRP", "MAC layer RSRP threshold", MAC_RSRPthreshold); 
  cmd.AddValue ("PDB", "Packet Delay Budget (PDB)", inputPDB); 

  cmd.AddValue ("Numerology", "Configure the OFDM numerology", OFDM_numerology);
  cmd.AddValue ("Periodic", "Enable periodic traffic generation", PeriodicTraffic);
  cmd.AddValue ("Aperiodic", "Enable aperiodic traffic generation", AperiodicTraffic);
  cmd.AddValue ("Mixed", "Enable the mixed periodic/aperiodic traffic generation", MixedTraffic);
  cmd.AddValue ("Percentage", "In mixed mode, the percentage of periodic UEs", PeriodicPercentage);

  cmd.AddValue ("ETSI", "Enable the ETSI-Algorithm for the CAMs generation", ETSITraffic);

  cmd.AddValue ("AvgRRI", "Reserve resources with average RRI in case of aperiodic traffic", avgRRI);

  cmd.AddValue ("FreqReuse", "Enable the frequency-reuse strategy", FrequencyReuse); 
  cmd.AddValue ("ReuseDist", "Set the reuse distance", ReuseDistance); 


 // cmd.AddValue ("Sens", "The reference sensitivity", RefSensitivity); 

  cmd.Parse(argc, argv);

  if (SCS_factor.find(OFDM_numerology) == SCS_factor.end())
    NS_ASSERT_MSG(false, "Non-valid OFDM numerology configuration");
  else
    NS_LOG_UNCOND("Adopted OFDM numerology is " << (uint32_t) OFDM_numerology << ", SCS = " << (15*SCS_factor[OFDM_numerology]) << " kHz");
  NS_ASSERT_MSG(OFDM_numerology != 3, "120 kHz SCS is not supported in FR1");


  channelBW_RBs = GetRbsFromBW(15*SCS_factor[OFDM_numerology], channelBW);

  NS_ASSERT_MSG(subchannelSize > 0, "Subchannel size must be larger than zero");
  NS_ASSERT_MSG(channelBW_RBs >= subchannelSize, "Channel bandwidth must be larger than the subchannel size");

  NS_LOG_UNCOND("Channel BW = " << channelBW << " MHz. Channel BW = " << channelBW_RBs << " RBs. Subchannel size = " << subchannelSize << " RBs");

  //RefSensitivity = GetRefSensitivity(15*SCS_factor[OFDM_numerology], channelBW);
  RefSensitivity = -103.5;
  
//  CBR_RSSIthreshold = GetRSSIthreshold(RefSensitivity);
  CBR_RSSIthreshold = -88.0;

  NS_LOG_UNCOND("UE reference sensitivity = " << RefSensitivity << " dBm, RSSI threshold = " << CBR_RSSIthreshold << " dBm");

  std::map < uint16_t, std::vector < std::pair <double, double>>>  SubchannelIndexMap;
  uint32_t NumGeoCells; 

  if (FrequencyReuse)
  {
    NS_ASSERT_MSG(ReuseDistance != 0, "Reuse distance must be larger than zero. Set it using --ReuseDist option");
    GeoCellSize = GetGeoCellSize(subchannelSize,channelBW_RBs,ReuseDistance,&NumGeoCells);
    NS_LOG_UNCOND("Reuse distance " << ReuseDistance << " meters. Geo-cell size " << GeoCellSize << " meters, number of geo-cells: " << NumGeoCells);
    // Number of geo-cells coincides with the number of subchannels
    double X_coord = 0.0;
    uint32_t subchannel_index = 0;
    while (X_coord <= highwayLength)
    {
      NS_LOG_DEBUG("Assigning subchannel index " << subchannel_index % NumGeoCells << " to geo-cell [" << X_coord << "," << X_coord + GeoCellSize << ")");
      std::map < uint16_t, std::vector < std::pair <double, double>>>::iterator geoIT = SubchannelIndexMap.find(subchannel_index % NumGeoCells);
      if (geoIT != SubchannelIndexMap.end()) 
      {
        SubchannelIndexMap[subchannel_index % NumGeoCells].push_back(std::pair<double, double> (X_coord, X_coord + GeoCellSize));
      }
      else
      {
        std::pair <double, double> tmpPair(X_coord, X_coord + GeoCellSize);
        std::vector < std::pair <double, double>> tmpVector = {tmpPair};
        SubchannelIndexMap.insert(std::pair < uint16_t, std::vector < std::pair <double, double>>> (subchannel_index % NumGeoCells, tmpVector));
      }
      X_coord += GeoCellSize;
      subchannel_index++;
    }   
  }

  /*Ptr<NrV2XAmc> NRamc = CreateObject <NrV2XAmc> ();
  for (uint16_t i=1; i< 20; i++)
  {
    uint16_t TBlen_subCH, TBlen_RBs;
    NRamc->GetSlSubchAndTbSizeFromMcs(i*100-3, mcs, subchannelSize, channelBW_RBs, &TBlen_subCH, &TBlen_RBs);
    NS_LOG_UNCOND("Pkt size = " << i*100 << " B requires " << TBlen_RBs << " RBs. Subchannel length = " << TBlen_subCH << " RBs");
  }
  std::cin.get(); 

  for (uint16_t i=0; i< 6; i++)
  {
    uint16_t TBlen_subCH, TBlen_RBs;
    NRamc->GetSlSubchAndTbSizeFromMcs(200+i*200, mcs, subchannelSize, channelBW_RBs, &TBlen_subCH, &TBlen_RBs);
    NS_LOG_UNCOND("Pkt size = " << 200+i*200 << ", RBs " << TBlen_RBs << " Subchannels = " << TBlen_subCH);
  }

  uint16_t TBlen_subCH, TBlen_RBs;
  NRamc->GetSlSubchAndTbSizeFromMcs(156+34, mcs, subchannelSize, channelBW_RBs, &TBlen_subCH, &TBlen_RBs);
  NS_LOG_UNCOND("Pkt size = " << 190 << ", RBs " << TBlen_RBs << " Subchannels = " << TBlen_subCH);
  NRamc->GetSlSubchAndTbSizeFromMcs(266+34, mcs, subchannelSize, channelBW_RBs, &TBlen_subCH, &TBlen_RBs);
  NS_LOG_UNCOND("Pkt size = " << 300 << ", RBs " << TBlen_RBs << " Subchannels = " << TBlen_subCH);
  std::cin.get();*/

  //Check the traffic configuration
  if (ETSITraffic)
  {
    if ((MixedTraffic) || (AperiodicTraffic) || (PeriodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model");      
  }
  else if (MixedTraffic)
  {
    NS_ASSERT_MSG((PeriodicPercentage == 10) || (PeriodicPercentage == 25) || (PeriodicPercentage == 50) || 
(PeriodicPercentage == 75) || (PeriodicPercentage == 90), "Non-valid periodic traffic percentage. Allowed values are 25, 50 and 75%");
  }
  else if (AperiodicTraffic)
  {
    if ((ETSITraffic) || (MixedTraffic) || (PeriodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model");      
  }
  else if (PeriodicTraffic)
  {
    if ((ETSITraffic) || (MixedTraffic) || (AperiodicTraffic))
      NS_ASSERT_MSG (false, "Enable only one traffic model"); 
  }
  else     
    NS_ASSERT_MSG (false, "Enable at least one traffic model"); 


  if (AdaptiveSchedulingMode2)
  {
    NS_ASSERT_MSG(MixedTraffic, "Adaptive scheduling is allowed only in mixed traffic scenarios");
    NS_ASSERT_MSG(!DynamicSchedulingMode2, "With mixed traffic only dynamic or adaptive scheduling is allowed, not both");
  }

 // NS_ASSERT_MSG (!(ExponentialModel and CAMtraceModel), "Choose only one aperiodic traffic model");
//  NS_ASSERT_MSG (!(ML_CAMtrace and GT_CAMtrace), "Choose the Machine Learning algorithm OR the Ground-Truth predictions"); // Da rimuovere per WNS3??

  if (RxCresel)
    NS_ASSERT_MSG(false, "Rx Cresel is not available");


  //------------------- Configure the output path ---------------------------------------------------------
  
  if (PeriodicTraffic)
    outputPath = "results/Periodic_Dynamic" + std::to_string(DynamicSchedulingMode2) + "_avgRRI" + std::to_string(avgRRI) + "_VariableSize" + std::to_string(VariablePacketSize) + "_ReEval" + std::to_string(ReEvaluation) + "_" + std::to_string(ueCount) + "_PDB" + std::to_string(inputPDB) +  "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 
  else if (AperiodicTraffic)
    outputPath = "results/Aperiodic_Dynamic" + std::to_string(DynamicSchedulingMode2) + "_avgRRI" + std::to_string(avgRRI) + "_VariableSize" + std::to_string(VariablePacketSize) + "_ReEval" + std::to_string(ReEvaluation) + "_" + std::to_string(ueCount) + "_PDB" + std::to_string(inputPDB) + "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 
  else
    if (AdaptiveSchedulingMode2)
      outputPath = "results/MixedAdaptive_Dynamic" + std::to_string(DynamicSchedulingMode2) + "_avgRRI" + std::to_string(avgRRI) + "_VariableSize" + std::to_string(VariablePacketSize) + "_ReEval" + std::to_string(ReEvaluation) + "_" + std::to_string(ueCount) + "_PDB" + std::to_string(inputPDB) + "_Perc" + std::to_string(PeriodicPercentage) + "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 
    else
      outputPath = "results/Mixed_Dynamic" + std::to_string(DynamicSchedulingMode2) + "_avgRRI" + std::to_string(avgRRI) + "_VariableSize" + std::to_string(VariablePacketSize) + "_ReEval" + std::to_string(ReEvaluation) + "_" + std::to_string(ueCount) + "_PDB" + std::to_string(inputPDB) + "_Perc" + std::to_string(PeriodicPercentage) + "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 

  //--------------------------------------------------------------------------------------------------------

/*
  if (PeriodicTraffic)
    outputPath = "results/Periodic_Dynamic" + std::to_string(DynamicSchedulingMode2) + "_FreqReuse" + std::to_string(FrequencyReuse) + "_" + std::to_string(channelBW) + "_" + std::to_string(ReuseDistance) + "_" + std::to_string(ueCount) + "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 
  else if (AperiodicTraffic)
    outputPath = "results/Aperiodic_Dynamic" + std::to_string(DynamicSchedulingMode2) + "_FreqReuse" + std::to_string(FrequencyReuse) + "_" + std::to_string(channelBW) + "_" + std::to_string(ReuseDistance) + "_" + std::to_string(ueCount) + "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 
  else
    outputPath = "results/sidelink_" + std::to_string(ueCount) + "_" + std::to_string(seed) + "_" + std::to_string(runNumber) + "/"; 
*/

  FilePath = outputPath;
  //Clear the results folder 

  system(("rm -r " + outputPath).c_str());
  system(("mkdir " + outputPath).c_str());

  std::ofstream readme;
  readme.open (outputPath + "simREADME.txt");
  readme << "----------------------" << std::endl;
  readme << "Simulation:" << std::endl;
  readme << "SUMO ON:" << g_sumoOn << std::endl;
  readme << "Trace file name:" << traceFileName << std::endl;
  readme << " - simulation time = " << simTime << " s " << std::endl;
  readme << " - seed = " << seed << std::endl; 
  readme << " - run = " << runNumber << std::endl;
  readme << " - UE count = " << ueCount << std::endl;
  readme << " - MCS = " << mcs << std::endl;
  readme << " - OFDM numerology index = " << OFDM_numerology << std::endl;
  readme << " - IBE = " << IBE << std::endl;
  readme << " - SubChannel size = " << subchannelSize << " RBs" << std::endl;
  readme << " - Channel BW = " << channelBW << " MHz. Channel BW = " << channelBW_RBs << " RBs" << std::endl;
  readme << " - Re-evaluation = " << ReEvaluation << std::endl;
  readme << " --- UMH variant = " << UMH_ReEvaluation << std::endl;
  readme << " --- All slots variant = " << AllSlots_ReEvaluation << std::endl;
  readme << " - Re-tranmissions = " << ReTranmissions << std::endl;
  readme << " - Periodic traffic = " << PeriodicTraffic << std::endl;
  readme << " - Aperiodic traffic = " << AperiodicTraffic << std::endl;
  readme << " - Mixed traffic = " << MixedTraffic << std::endl;
  if (MixedTraffic)
  {
    readme << " --- Periodic users = " << PeriodicPercentage << "%" << std::endl;
    readme << " --- Adaptive scheduling " << AdaptiveSchedulingMode2 << std::endl;
  }
  if (VariablePacketSize)
    readme << " - Variable packet size " << std::endl;
  else
    readme << " - Fixed packet size " << std::endl;

  if (inputPDB != 0)
    readme << " --- Custom PDB set to " << inputPDB << " ms" << std::endl;
  
  readme << " - MAC layer:" << std::endl;
  readme << " --- Dynamic scheduling " << DynamicSchedulingMode2 << std::endl;

  if (avgRRI)
    readme << " --- Average RRI " << std::endl;
  else 
    readme << " --- Minimum RRI " << std::endl;   


  readme << " - RSRP threshold = " << MAC_RSRPthreshold << " dBm" << std::endl;
  readme << " - RSSI threshold = " << CBR_RSSIthreshold << " dBm" << std::endl;
  readme << " - PHY sensitivity = " << RefSensitivity << " dBm" << std::endl;

  if (FrequencyReuse)
  {
    readme << " --- Frequency-reuse enabled " << std::endl;
    readme << " ----- Reuse distance = " << ReuseDistance << " meters" << std::endl;
    readme << " ----- Geo-cell size = " << GeoCellSize << " meters" << std::endl;
    readme << " ----- Geo-cells number = " << NumGeoCells << std::endl;
  }
  else
    readme << " --- Frequency-reuse disabled " << std::endl;
  readme << "Pkeep: " << pkeep << std::endl;

  readme.close ();


  // ---------------------- Output the frequency reuse map ----------------------
  if (FrequencyReuse)
  {
    std::ofstream FreqReuseMap;
    FreqReuseMap.open (outputPath + "FreqReuseMap.txt");
    for (std::map < uint16_t, std::vector < std::pair <double, double>>>::iterator mapIT = SubchannelIndexMap.begin(); mapIT != SubchannelIndexMap.end(); mapIT++)
    {
  //    NS_LOG_UNCOND("Subchannel index " << mapIT->first << " includes the following geo-cells:");
      std::vector < std::pair <double, double>> GeoCellsVector = mapIT->second;
      for(std::vector < std::pair <double, double>>::iterator GeoCellsIT = GeoCellsVector.begin(); GeoCellsIT != GeoCellsVector.end(); GeoCellsIT++)
      {
//        NS_LOG_UNCOND("--- [" << GeoCellsIT->first << "," << GeoCellsIT->second << ")");
        FreqReuseMap << GeoCellsIT->first << "," << GeoCellsIT->second << "," << mapIT->first << std::endl;
      }
    }
    FreqReuseMap.close();
  //  std::cin.get();  
  }


// Set the random seed and run
  RngSeedManager::SetSeed (seed);
  RngSeedManager::SetRun (runNumber);


  //Initialize the position checker
  PositionChecker.initPolygon(polygonTX, (int)sizeof(polygonTX)/sizeof(polygonTX[0]), "TX"); //Filter TX users
  PositionChecker.initPolygon(polygonRX, (int)sizeof(polygonRX)/sizeof(polygonRX[0]), "RX"); //Filter RX users
  PositionChecker.initBBs(BBs); 
  PositionChecker.DisableChecker(); // Disable the UEs position checker


// Using the ns3::Config::* Functions to set up the simulations
  NS_LOG_INFO ("Configuring UE settings...");

  // Configuring MAC sublayer
  Config::SetDefault ("ns3::NrV2XUeMac::SlGrantMcs", UintegerValue (mcs)); //The MCS of the SL grant, must be [0..15] (default 0)
  Config::SetDefault ("ns3::NrV2XUeMac::ListL2Enabled", BooleanValue (false)); 

  Config::SetDefault ("ns3::NrV2XUeMac::MixedTraffic", BooleanValue (MixedTraffic)); 
  Config::SetDefault ("ns3::NrV2XUeMac::AllowReEvaluation", BooleanValue (ReEvaluation)); 
  Config::SetDefault ("ns3::NrV2XUeMac::AllSlotsReEvaluation", BooleanValue (AllSlots_ReEvaluation));
  Config::SetDefault ("ns3::NrV2XUeMac::RSRPthreshold", DoubleValue (MAC_RSRPthreshold));
  Config::SetDefault ("ns3::NrV2XUeMac::EnableReTx", BooleanValue (ReTranmissions));
  Config::SetDefault ("ns3::NrV2XUeMac::DynamicScheduling", BooleanValue (DynamicSchedulingMode2));
  Config::SetDefault ("ns3::NrV2XUeMac::AdaptiveScheduling", BooleanValue (AdaptiveSchedulingMode2));
  Config::SetDefault ("ns3::NrV2XUeMac::UMHReEvaluation", BooleanValue (UMH_ReEvaluation));
  Config::SetDefault ("ns3::NrV2XUeMac::FrequencyReuse", BooleanValue (FrequencyReuse));

  // Pkeep
  Config::SetDefault ("ns3::NrV2XUeMac::Pkeep", DoubleValue(pkeep));


  // Configure Power Control and Phy layer
  Config::SetDefault ("ns3::NrV2XUePhy::TxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::Pcmax", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PoNominalPusch", IntegerValue (-106));
  Config::SetDefault ("ns3::NistLteUePowerControl::PscchTxPower", DoubleValue (ueTxPower));  // Setting the Transmission Power for the control channel
  Config::SetDefault ("ns3::NistLteUePowerControl::PsschTxPower", DoubleValue (ueTxPower));  // Setting the Transmission Power for the sidelink channel
  Config::SetDefault ("ns3::NrV2XUePhy::RsrpUeMeasThreshold", DoubleValue (-10.0));        // Setting the RSRP threshold
  Config::SetDefault ("ns3::NrV2XUePhy::ReferenceSensitivity", DoubleValue (RefSensitivity));  
  Config::SetDefault ("ns3::NrV2XUePhy::RSSIthreshold", DoubleValue (CBR_RSSIthreshold));  

  // Configure spectrum layer
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::ReferenceSensitivity", DoubleValue (RefSensitivity));  
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (CtrlErrorModelEnabled));  // Set but not used
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (!CtrlErrorModelEnabled)); // Set but not used
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SaveCollisionLossesUnimore", BooleanValue (true)); //fare var apposta   // Enable the collision and propagation loss event saving

  // Used for 
  Config::SetDefault ("ns3::NrV2XUeMac::RandomV2VSelection", BooleanValue (randomV2VSelection));

  NS_LOG_INFO ("Starting network configuration...");

  Config::SetDefault ("ns3::NistLteRlcUm::MaxTxBufferSize", StringValue ("100000"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (10000000)); // In order not to run out of packets ;)
 

  // Configure the path for storing output files--------------------------------------------------------------------
  Config::SetDefault ("ns3::NistLteRlcUm::OutputPath", StringValue (outputPath)); 
  Config::SetDefault ("ns3::NrV2XUeMac::OutputPath", StringValue (outputPath)); 
  Config::SetDefault ("ns3::NrV2XUePhy::OutputPath", StringValue (outputPath)); 
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::OutputPath", StringValue (outputPath)); 
  // Configure the saving period------------------------------------------------------------------------------------
  // Tradeoff between speed of the simulation and memory requirements
  double SavingPeriod = 2.0;
  Config::SetDefault ("ns3::NrV2XUeMac::SavingPeriod", DoubleValue (SavingPeriod)); 
  Config::SetDefault ("ns3::NrV2XUePhy::SavingPeriod", DoubleValue (SavingPeriod)); 
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SavingPeriod", DoubleValue (SavingPeriod)); 
  //-------------------------------------------------------------------------------------------------------



  // Configuration of NR-V2X numerologies--------------------------------------------------------------------
  Config::SetDefault ("ns3::NrV2XUeMac::SubchannelSize", UintegerValue (subchannelSize)); 
  Config::SetDefault ("ns3::NrV2XUeMac::RBsBandwidth", UintegerValue (channelBW_RBs)); 
  Config::SetDefault ("ns3::NrV2XUeMac::SlotDuration", DoubleValue (1.0/SCS_factor[OFDM_numerology])); 
  Config::SetDefault ("ns3::NrV2XUeMac::NumerologyIndex", UintegerValue (OFDM_numerology)); 

  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubchannelSize", UintegerValue (subchannelSize)); 
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::RBsBandwidth", UintegerValue (channelBW_RBs)); 
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SlotDuration", DoubleValue (1.0/SCS_factor[OFDM_numerology])); 

  switch (OFDM_numerology)
  {
    case 0:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration", TimeValue(NanoSeconds (1e6 - 71350 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue(15));  
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue(15));  
      break;
    case 1:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration", TimeValue(NanoSeconds (0.5e6 - 35680 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue(30));
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue(30));  
      break;
    case 2:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration", TimeValue(NanoSeconds (0.25e6 - 17840 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue(60));
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue(60));  
      break;
    case 3:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration", TimeValue(NanoSeconds (0.125e6 - 8920 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue(120));
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue(120));  
      break;
  };

  Config::SetDefault ("ns3::NrV2XUePhy::SlotDuration", DoubleValue (1.0/SCS_factor[OFDM_numerology])); 
  Config::SetDefault ("ns3::NrV2XUePhy::SubchannelSize", UintegerValue (subchannelSize)); 
  Config::SetDefault ("ns3::NrV2XUePhy::RBsBandwidth", UintegerValue (channelBW_RBs)); 
  Config::SetDefault ("ns3::NrV2XUePhy::IBE", BooleanValue (IBE)); 
//  std::cin.get();
  Config::SetDefault ("ns3::NistLtePhy::TTI", DoubleValue ( 1.0/(1000*SCS_factor[OFDM_numerology]) )); 
  
  //Config::SetDefault ("ns3::NrV2XAmc::PSSCH_DMRStimePattern", StringValue ("{2,3,4}")); 


  //-------------------------------------------------------------------------------------------------------

  // These two lines are not used. Needed only to configure the simulation using a stored input configuration file
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();  


  NS_LOG_INFO ("Creating helpers...");
//@DOC Not really needed: ns3 must set up the LTE infrastructure to run the simulation (at least the PDN-GW or PGW to assign IP addresses)
  Ptr<NistPointToPointEpcHelper>  epcHelper = CreateObject<NistPointToPointEpcHelper> (); // NIST EPC helper, used later
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  Ptr<NistLteHelper> lteHelper = CreateObject<NistLteHelper> ();  // Important! It's the NIST modified version of a lteHelper, which is needed to 
								  // create and configure LTE network entities (see online Doc)

  //@NEW
  //@DOC: now setting the operating frequency [in GHz]
  // Set 3gpp pathloss model
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::Frequency", DoubleValue (5.9));
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::Sigma", DoubleValue (3.0));
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::SigmaNLOSv", DoubleValue (4.0));
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::DecorrDistance", DoubleValue (25.0));

  lteHelper->SetPathlossModelType ("ns3::NrV2XPropagationLossModel"); 

  Ptr<NrV2XPropagationLossModel> Sl3GPPChannelMatrix = CreateObject<NrV2XPropagationLossModel> ();

  Config::SetDefault ("ns3::NrV2XSpectrumPhy::ChannelMatrix", PointerValue (Sl3GPPChannelMatrix)); 

 


  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));  // ObjectBase::SetAttribute() sets a single attribute, raising fatal errors if unsuccessful
  lteHelper->SetEpcHelper (epcHelper);  // Set the NIST ECP helper as lteHelper

  lteHelper->Initialize (); // Invoke DoInitialize() on all Objects aggregated to this one

  Ptr<NistLteProseHelper> proseHelper = CreateObject<NistLteProseHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  NS_LOG_INFO ("Deploying UE's...");


//@DOC: Now the UEs (actually, V-UEs are created)
  NodeContainer ueResponders;
 
   
  for (uint32_t t=0; t<ueCount; ++t)
  {
    Ptr<Node> lteNode = CreateObject<Node> ();
    // LTENodeState is a new class developed from scratch. Defined before the main()
    Ptr<LTENodeState> nodeState = CreateObject<LTENodeState> ();
    nodeState -> SetNode(lteNode);
    lteNode -> AggregateObject(nodeState);  // Aggregate two objects together. Now it's possible to call GetObject() on one to get the other and vice-versa.
    ueResponders.Add(lteNode);
  }
 
  MobilityHelper mobilityUE;

  Ptr<UniformRandomVariable> laneNumber = CreateObject<UniformRandomVariable>();
  Ptr<UniformRandomVariable> Xposition = CreateObject<UniformRandomVariable>();
  double laneWidth = 4.0;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();

  //int pospos = 1;
  //GeoCellSize = 8;
  for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
  {  
    double yPos = laneNumber->GetInteger(1,6)*laneWidth;
//    double yPos = laneNumber->GetInteger(1,1)*laneWidth; // 1 lane simulation
    double xPos = Xposition->GetValue(0,highwayLength);
//    double xPos = Xposition->GetValue(0,ReuseDistance);
//    double xPos = pospos;
//    pospos += GeoCellSize;
//    xPos += 2000;
    positionAlloc ->Add(Vector(xPos, yPos, 0)); 
  }
  mobilityUE.SetPositionAllocator(positionAlloc);
  mobilityUE.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobilityUE.Install (ueResponders);
  if (g_sumoOn && traceFileName!=""){
    opencsv(traceFileName);
    UpdateVehiclePositionsFromTrace(&traceFile, ueResponders);
    // SUMO position calculated
  }
  else{
  //mobilityUE->SetVelocity({20,0,0});

  for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
  {  
    Ptr<Node> node = *L;
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
    Ptr<ConstantVelocityMobilityModel> VelMob = node->GetObject<ConstantVelocityMobilityModel>();
    if (mob->GetPosition().y > 13)
      VelMob->SetVelocity(Vector(19.44, 0, 0));     
//      VelMob->SetVelocity(Vector(0, 0, 0));     
    else
      VelMob->SetVelocity(Vector(-19.44, 0, 0));
//      VelMob->SetVelocity(Vector(0, 0, 0));          
  }
  }
  
/*  MobilityHelper mobilityUE;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); 
  positionAlloc ->Add(Vector(10, 0, 0)); 
  positionAlloc ->Add(Vector(20, 0, 0)); 
  positionAlloc ->Add(Vector(30, 0, 0)); 

  mobilityUE.SetPositionAllocator(positionAlloc);
  mobilityUE.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityUE.Install (ueResponders); */


  for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
  {  
    int ID;
    Ptr<Node> node = *L;
    ID = node->GetId ();
  //   std::cin.get();
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
    if (! mob) continue; // Strange -- node has no mobility model installed. Skip.
    Vector pos = mob->GetPosition ();

    PrevX.push_back(pos.x);
    PrevY.push_back(pos.y);
    PrevZ.push_back(pos.z);

    VelX.push_back(0);
    VelY.push_back(0);
    VelZ.push_back(0);
//     PrevX[ID-1] = pos.x;
//     PrevY[ID-1] = pos.y;
//     PrevZ[ID-1] = pos.z;


    if ((AperiodicTraffic) || (ETSITraffic))
//      VehicleTrafficType[ID-1] = 0x01;  // 0x00 for periodic traffic. 0x01 for aperiodic traffic
      VehicleTrafficType.push_back(0x01);
    else if (PeriodicTraffic)
//      VehicleTrafficType[ID-1] = 0x00;
      VehicleTrafficType.push_back(0x00);
    else if (PeriodicPercentage == 10)
    {
      if (ID % 10 == 0)       
      	VehicleTrafficType.push_back(0x00);
      else
      	VehicleTrafficType.push_back(0x01);
    }
    else if (PeriodicPercentage == 25)
    {
      if (ID % 4 == 0)       
      	VehicleTrafficType.push_back(0x00);
      else
      	VehicleTrafficType.push_back(0x01);
    }
    else if (PeriodicPercentage == 50)
    {
//      VehicleTrafficType[ID-1] = 0x01;
      if (ID % 2 == 0)       
         VehicleTrafficType.push_back(0x00);
      else
         VehicleTrafficType.push_back(0x01);
    }
    else if (PeriodicPercentage == 75)
    {
      if (ID % 4 == 0)       
      	VehicleTrafficType.push_back(0x01);
      else
      	VehicleTrafficType.push_back(0x00);
    }
    else if (PeriodicPercentage == 90)
    {
      if (ID % 10 == 0)       
      	VehicleTrafficType.push_back(0x01);
      else
      	VehicleTrafficType.push_back(0x00);
    }

//    EnableTX[ID-1] = true; //Enable a UE to transmit
    EnableTX.push_back(true); //Enable a UE to transmit
    
  }

 Print(ueResponders);  // Print the initial position of the nodes in the output file
   

  Sl3GPPChannelMatrix->InitChannelMatrix(ueResponders);

  ModuloSplit = 10;

  //LargestAperiodicSize, LargestPeriodicSize;
   // Define the packets inter-arrival time and size
   if ((MixedTraffic) || (AperiodicTraffic) || (PeriodicTraffic))
   {
     //@LUCA setting the random variable generator for working with aperiodic traffic
     //Tgen_aperiodic_c = 50;
     RndExp = CreateObject<ExponentialRandomVariable> ();
     RndExp_1 = CreateObject<ExponentialRandomVariable> ();


//     uint16_t quantizationStep = 100;
     uint16_t quantizationStep = 200;
     LargestAperiodicSize = 1200; // Largest packet size for aperiodic traffic
//     LargestAperiodicSize = 100; // Largest packet size for aperiodic traffic
     for(uint16_t k = 1; k <= LargestAperiodicSize/quantizationStep; k++)
     {
       if (VariablePacketSize)
         AperiodicPKTs_Size.push_back(k*quantizationStep-35); 
       else
         AperiodicPKTs_Size.push_back(200-35);  // Valid packet sizes from 100 to 1000 bytes with 100 bytes quantization step
     }

     for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
     {
       int ID;
       Ptr<Node> node = *L;
       ID = node->GetId ();
//       if (ID % ModuloSplit != 0)   
       if (ID % ModuloSplit <= 1)   
       {
         if (inputPDB != 0)
           PDB_Aperiodic.push_back(inputPDB);
         else
           PDB_Aperiodic.push_back(10);
//           PDB_Aperiodic.push_back(10);
         Aperiodic_Tgen_c.push_back(10);
//         Aperiodic_Tgen_c.push_back(10);
         RndExp->SetAttribute ("Mean", DoubleValue(10));
//         RndExp->SetAttribute ("Mean", DoubleValue(10));
//         Aperiodic_Tgen_c.push_back(5);
//         RndExp->SetAttribute ("Mean", DoubleValue(5));
       }
       else
       {
         if (inputPDB != 0)
           PDB_Aperiodic.push_back(inputPDB);
         else
           PDB_Aperiodic.push_back(50);
//           PDB_Aperiodic.push_back(10);
         Aperiodic_Tgen_c.push_back(50);
//         Aperiodic_Tgen_c.push_back(10);
         RndExp_1->SetAttribute ("Mean", DoubleValue(50));
//         RndExp_1->SetAttribute ("Mean", DoubleValue(10));

//         Aperiodic_Tgen_c.push_back(10);
//         RndExp_1->SetAttribute ("Mean", DoubleValue(10));

//         Aperiodic_Tgen_c.push_back(5);
//         RndExp_1->SetAttribute ("Mean", DoubleValue(5));

//         Aperiodic_Tgen_c.push_back(25);
//         RndExp_1->SetAttribute ("Mean", DoubleValue(25));
       }
     } 


     for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
     {
       int ID;
       Ptr<Node> node = *L;
       ID = node->GetId ();
//       if (ID % 2 == 0)   
       if (ID % ModuloSplit <= 1)   
       {
         if (inputPDB != 0)
           PDB_Periodic.push_back(inputPDB);
         else
           PDB_Periodic.push_back(20);
//           PDB_Periodic.push_back(20);
         Periodic_Tgen.push_back(20);
//         Periodic_Tgen.push_back(20);
       }
       else
       {
         if (inputPDB != 0)
           PDB_Periodic.push_back(inputPDB);
         else
           PDB_Periodic.push_back(100);
//           PDB_Periodic.push_back(20);
         Periodic_Tgen.push_back(100);
//         Periodic_Tgen.push_back(20);
       }
     } 
//     PeriodicPKTs_Size = {190-34, 190-34, 190-34, 190-34 ,190-34}; //Account for the overhead
//     PeriodicPKTs_Size = {300-34, 190-34, 190-34, 190-34 ,190-34}; //Account for the overhead
//     PeriodicPKTs_Size = {300-34, 300-34, 300-34, 300-34 ,300-34}; //Account for the overhead
     PeriodicPKTs_Size = {200-35, 200-35, 200-35, 200-35, 200-35}; //Account for the overhead
     LargestPeriodicSize = PeriodicPKTs_Size[0];  // 300 bytes is the largest packet size for aperiodic traffic

     Ptr<UniformRandomVariable> random_index = CreateObject<UniformRandomVariable>();
     for (NodeContainer::Iterator NN = ueResponders.Begin(); NN != ueResponders.End(); ++NN)  // UEs are indexed starting from 1
     {  
       Pattern_index.push_back(random_index->GetInteger(0,PeriodicPKTs_Size.size()-1) );
     }
   }
   else //It's ETSI traffic
   {
     for (NodeContainer::Iterator NN = ueResponders.Begin(); NN != ueResponders.End(); ++NN)
     {  
       Pattern_index.push_back(0);
     }
     LargestCAMSize = 850;
//     system("CAM-tools/test.sh");

     system(("cd src/MoReV2X/CAM-tools/CAM-model/ && python3 NS3_traces_generation.py -p CAMtraces --model Complete --scenario Highway --profile Volkswagen -m 5 -n " + std::to_string(ueCount) + " -t 30 ").c_str());

//     std::cin.get();
     LoadCAMtraces(ueResponders);

   }
   
   MeasInterval = ((double)TrepPrint)/1000;

  //mobility.SetPositionAllocator (positionAlloc);

  NS_LOG_INFO ("Installing UE network devices...");
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueResponders);

  for (NodeContainer::Iterator L = ueResponders.Begin(); L != ueResponders.End(); ++L)
  {
    Ptr<Node> node = *L;
    NS_LOG_INFO("Node " << node->GetId() << ", " << node->GetNDevices());
    Ptr<NetDevice> dev = node->GetDevice(0);
    Ptr<NistLteUeNetDevice> netDev = dev->GetObject<NistLteUeNetDevice>();
    Ptr<NrV2XUeMac> mac = netDev->GetMac();

    if (FrequencyReuse)
      mac->CopySubchannelsMap(SubchannelIndexMap);

    if (PeriodicTraffic)
    {
      mac->PushNewRRIValue(100);
    }
    else if (AperiodicTraffic)
    {
      if (avgRRI) 
      {
        mac->PushNewRRIValue(RndExp->GetMean()*2);
        mac->PushNewRRIValue(RndExp_1->GetMean()*2);
      }
      else
      {
        mac->PushNewRRIValue(RndExp->GetMean());
        mac->PushNewRRIValue(RndExp_1->GetMean());
      }
    }
    else  // Mixed traffic
    {
      mac->PushNewRRIValue(100);
//      mac->PushNewRRIValue(20);
      if (avgRRI) 
      {
        mac->PushNewRRIValue(RndExp->GetMean()*2);
        mac->PushNewRRIValue(RndExp_1->GetMean()*2);
      }
      else
      {
        mac->PushNewRRIValue(RndExp->GetMean());
        mac->PushNewRRIValue(RndExp_1->GetMean());
      }
    }
//      mac->PushNewRRIValue(100);
//      mac->PushNewRRIValue(20);
//      mac->PushNewRRIValue(50);
//      mac->PushNewRRIValue(10);
    
  }

   // NetDeviceContainer ueSendersDevs = lteHelper->InstallUeDevice (ueResponders);
   // ueDevs.Add (ueSendersDevs);


  NS_LOG_INFO ("Installing IP stack...");
  InternetStackHelper internet;
  internet.Install (ueResponders);
  
  NS_LOG_INFO ("Allocating IP addresses and setting up network route...");

  /*Ipv4InterfaceContainer ueIpIface;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  ueIpIface = epcHelper->AssignUeIpv4Address (ueDevs);*/

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");

  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer intcont = ipv4.Assign (ueDevs);
    //Ipv4InterfaceContainer intcont2 = ipv4.Assign (ueSendersDevs); 

  // Set up the IP network seen from the eNB
  for (uint32_t u = 0; u < ueResponders.GetN (); ++u) // Could have used an iterator also here. Anyway, the result is the same
    {
      Ptr<Node> ueNode = ueResponders.Get (u);
//      std::cout << ueNode -> GetObject<Ipv4> () -> GetAddress(1,0).GetLocal() << std::endl;
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
     // ueStaticRouting->SetDefaultRoute (Ipv4Address::GetAny(), 1);
      //ueStaticRouting->SetDefaultMulticastRoute(1);
    }

  
  NS_LOG_INFO ("Attaching UE's to LTE network...");
  lteHelper->Attach (ueDevs);

  BuildingsHelper::Install (ueResponders);
  BuildingsHelper::MakeMobilityModelConsistent ();
  
  NS_LOG_INFO ("Installing applications...");
  // UDP application: in our case, this is the application that generates CAMs and DENMs

  //std::cin.get();

 TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // groupAddress is a Ipv4Address variable
//  Ipv4AddressGenerator::Init(Ipv4Address ("225.0.0.0"), Ipv4Mask ("255.0.0.0"));
//  groupAddress = Ipv4AddressGenerator::NextAddress (Ipv4Mask ("255.0.0.0"));
  groupAddress = "225.0.0.1";
  std::cout << "Group address " << groupAddress << std::endl;
  UdpClientHelper udpClient (groupAddress , 8000); //set destination IP address and UDP port (8000 in this case). The group address is used to set the Sidelink Bearers

  udpClient.SetAttribute ("MaxPackets", UintegerValue (100000));
  udpClient.SetAttribute ("Interval", TimeValue (MilliSeconds (100)));  // Useful only for periodic traffic
  udpClient.SetAttribute ("PacketSize", UintegerValue (423));  // Fake value, useless

  ApplicationContainer clientApps = udpClient.Install(ueResponders);
//  ApplicationContainer clientApps = udpClient.Install(ueResponders.Get(0));  // Similar to NodeContainer, but now is a vector of smart pointers pointing to applications
//  clientApps.Add(udpClient.Install(ueResponders.Get(1)));	
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();

//@OLD  clientApps.Get(0) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
  for(uint32_t i = 0; i < ueResponders.GetN(); i++)
  {
      //@DOC: Randomize the generation time of CAM messages    
      clientApps.Get(i) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
      clientApps.Get(i) -> SetStopTime (Seconds (simTime + 0.5)); // The simulation ends at simTime + 1, so we have a buffer of 0.5s 
  }

//  clientApps.Get(0) -> SetStartTime(Seconds (0.1));
//  clientApps.Get(1) -> SetStartTime(Seconds (0.6));
//  clientApps.Get(2) -> SetStartTime(Seconds (0.6));
//  clientApps.Get(0) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
//  clientApps.Get(0) -> SetStopTime (Seconds (simTime + 0.5)); // The simulation ends at simTime + 1, so we have a buffer of 0.5s 
//  clientApps.Get(1) -> SetStartTime(Seconds (rand -> GetValue(0.001, 1.0)));
//  clientApps.Get(1) -> SetStopTime (Seconds (simTime + 0.5));
//@OLD  clientApps.Get(0) -> SetStopTime (Seconds (simTime + 1));
 
  // Application to receive traffic
  PacketSinkHelper clientPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8000));  // Ipv4Address::GetAny() returns the dummy 0.0.0.0 address. The port number must be the correct one
  ApplicationContainer clientRespondersSrvApps = clientPacketSinkHelper.Install(ueResponders);
  clientRespondersSrvApps.Start (Seconds (0.001));  // Start ASAP
  clientRespondersSrvApps.Stop (Seconds (simTime+0.9));


  // At this point, each V-UE mounts both a Client and a Server (Sink) application
  
  NS_LOG_INFO ("Creating sidelink configuration...");
  uint32_t groupL2Address = 0x00;
  

  Ptr<NistSlTft> tftRx = Create<NistSlTft> (NistSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds(0.001), ueDevs, tftRx); // Set accordingly with clientApps StartTime! 0.1 Seconds is the ActivationTime of the Sidelink bearer


  Ptr<LteUeRrcSl> ueSidelinkConfiguration = CreateObject<LteUeRrcSl> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  NistLteRrcSap::SlPreconfiguration preconfiguration;
  preconfiguration.preconfigGeneral.carrierFreq = 54900; //not important
  preconfiguration.preconfigGeneral.slBandwidth = channelBW_RBs;    //original: 50 PRBs = 10MHz
  preconfiguration.preconfigComm.nbPools = 1; // the number of pools, not relevant for V2V

  NistSlPreconfigPoolFactory pfactory;
  NistSlResourcePoolFactory commfactory;
 

  //build PSCCH bitmap value
  uint64_t pscchBitmapValue = 0x0;
  for (uint32_t i = 0 ; i < pscchLength; i++) {
    pscchBitmapValue = pscchBitmapValue >> 1 | 0x8000000000;
  }
  std::cout << "bitmap=" << std::hex << pscchBitmapValue << '\n'; // this is the PSCCH subframe pool bitmap, from NIST D2D implementation

  pfactory.SetControlBitmap (pscchBitmapValue);
  pfactory.SetControlPeriod (period);
  pfactory.SetDataOffset (pscchLength);

  commfactory.SetControlBitmap (pscchBitmapValue);

  pfactory.SetHaveUeSelectedResourceConfig(false); // Mode1 --> done in this way, it does not work

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
 
  NS_LOG_INFO ("Installing sidelink configuration...");
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

//  std::cin.get();

  NS_LOG_INFO ("Starting simulation...");
  Simulator::Stop (Seconds (simTime+1)); 
  Simulator::Run ();
  /*
    Put code to evaluate KPIs here
  */
  Simulator::Destroy ();

  NS_LOG_INFO ("Done.");

  return 0;

}

