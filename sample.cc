#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("Server_App");

class Server_App : public Application 
{
	public:
		Server_App ();
		virtual ~Server_App ();
		uint16_t m_port;
		Ptr<Node> server_node;
		
	protected:
		virtual void DoDispose (void);

	private:

		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void HandleRead (Ptr<Socket> socket);
		int recv_packets;
		Ptr<Socket> m_socket;
		
		Address m_local;
};

Server_App::Server_App ()
{
	NS_LOG_FUNCTION_NOARGS ();
	recv_packets=0;
}

Server_App::~Server_App()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void Server_App::DoDispose (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void Server_App::StartApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	recv_packets=0;
	if (m_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
		m_socket->Bind (local);
		if (addressUtils::IsMulticast (m_local))
		{
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
			if (udpSocket)
			{
				// equivalent to setsockopt (MCAST_JOIN_GROUP)
				udpSocket->MulticastJoinGroup (0, m_local);
			}
			else
			{
				NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
			}
		}
		
		//MakeCallback(&Server_App::HandleRead, &m_socket);
		
		Ptr<Packet> p;
  
		uint8_t *buffer = new uint8_t [1024];
		buffer=(uint8_t *)"cmd";
		p = Create<Packet> (buffer,1024);
		
		
		TypeId tid1 = TypeId::LookupByName ("ns3::UdpSocketFactory");
		
		Ptr<Ipv4> server_ip = server_node->GetObject<Ipv4> (); 
		
		Ptr<ns3::Socket> beacon_source;
		beacon_source = Socket::CreateSocket (GetNode(), TypeId::LookupByName ("ns3::UdpSocketFactory"));
		InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 10);
		beacon_source->SetAllowBroadcast (true);
		beacon_source->Connect (remote);

		beacon_source->Send(p);
		
		
	}
	m_socket->SetRecvCallback (MakeCallback (&Server_App::HandleRead, this));
}

void Server_App::StopApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_LOG_INFO ("FINAL => Total Packets received :" << recv_packets);
	if (m_socket != 0) 
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
}

void Server_App::HandleRead (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;
	while (packet = socket->RecvFrom (from))
	{
		if (InetSocketAddress::IsMatchingType (from))
		{
		
			uint8_t *buffer = new uint8_t [1024]; 
			packet->CopyData (buffer, 1024); 
			
			
			//NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from client " <<
			      // InetSocketAddress::ConvertFrom (from).GetIpv4 () << " with contents " <<buffer);
			
			if(strcmp((char *)buffer,"cmd")!=0)
				recv_packets++;
		}
		
	}
}

class Client_App : public Application 
{
	public:
		Client_App ();
		virtual ~Client_App ();
		Ipv4Address m_peerAddress;
		uint16_t m_peerPort;
		uint32_t m_count;
		Time m_interval;
		uint32_t m_size;
		uint32_t hops;
		uint8_t recv_cmd;

	protected:
		virtual void DoDispose (void);

	private:

		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void HandleRead (Ptr<Socket> socket);
		
		uint32_t m_dataSize;
		uint8_t *m_data;

		uint32_t m_sent;
		Ptr<Socket> m_socket;
		
		EventId m_sendEvent;
};

Client_App::Client_App ()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_sent = 0;
	m_socket = 0;
	m_sendEvent = EventId ();
	m_data = 0;
	m_dataSize = 0;
	recv_cmd=0;
}

Client_App::~Client_App()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	delete [] m_data;
	m_data = 0;
	m_dataSize = 0;
}


void Client_App::DoDispose (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void Client_App::StartApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		m_socket->Bind ();
		m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
	}

	Ptr<ns3::Socket> beacon_sink;
	beacon_sink = Socket::CreateSocket (GetNode(), TypeId::LookupByName ("ns3::UdpSocketFactory"));
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 10);
	beacon_sink->Bind (local);

	beacon_sink->SetRecvCallback (MakeCallback (&Client_App::HandleRead, this));

	//ScheduleTransmit (Seconds (0.));
}

void Client_App::StopApplication ()
{
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket != 0) 
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		m_socket = 0;
	}

	Simulator::Cancel (m_sendEvent);
}




void
Client_App::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while (packet = socket->RecvFrom (from))
	{
		if (InetSocketAddress::IsMatchingType (from))
		{
			uint8_t *buffer = new uint8_t [1024]; 
			packet->CopyData (buffer, 1024); 
		
			if(strcmp ((char*)buffer,"cmd")==0)
			{
				if(recv_cmd==0)
				{
					//NS_LOG_INFO ("Received command of size " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () );
					//<< " with contents " <<buffer);
				       
					recv_cmd=1;

					Ptr<ns3::Socket> b_source;
					b_source = Socket::CreateSocket (GetNode(), TypeId::LookupByName ("ns3::UdpSocketFactory"));
					InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 10);
					b_source->SetAllowBroadcast (true);
					b_source->Connect (remote);

					packet->RemoveAllPacketTags ();
					packet->RemoveAllByteTags ();

					b_source->Send(packet);

					Ptr<Packet> p;

					uint8_t *buffer = new uint8_t [1024];
					char array[20];
					sprintf(array,"%u",hops);

					buffer = (uint8_t*)array;
					//buffer= (uint8_t *)hops;
					p = Create<Packet> (buffer,1024);

					//std::cout<<"content of client packet :"<<buffer<<"\n";
					b_source->Send(p);
				}
			}
			else
			{
	
		  //NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " <<
			    //   InetSocketAddress::ConvertFrom (from).GetIpv4 () << " with contents " <<buffer);
			       
			       
			       char *s= new char [20];
			       s=(char*) buffer;
			       int h = atoi(s);
			       
			       if(h!=0)
			       {
					h=h-1;
					char *q=new char[20];
			
					sprintf(q,"%u",h);
			
					buffer = (uint8_t*)q;      
					Ptr<Packet> p1;
					p1 = Create<Packet> (buffer,1024);
					Ptr<ns3::Socket> b_source1;
					b_source1 = Socket::CreateSocket (GetNode(), TypeId::LookupByName ("ns3::UdpSocketFactory"));
					InetSocketAddress remote1 = InetSocketAddress (Ipv4Address ("255.255.255.255"), 10);
					b_source1->SetAllowBroadcast (true);
					b_source1->Connect (remote1);

					b_source1->Send(p1);
							 
			       }
			 }      
		}
	}
}


int main (int argc, char *argv[])
{

	LogComponentEnable("Server_App", LOG_LEVEL_INFO);
	//LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
	uint32_t num_nodes = 500;
	
	NS_LOG_INFO ("Creating nodes");
	NodeContainer nodes;
	nodes.Create(num_nodes);
	NS_LOG_INFO ("Finished creating nodes");
	
	
	NS_LOG_INFO ("Installing Internet stack on nodes");
	InternetStackHelper internet;
 	internet.Install (nodes);
  	NS_LOG_INFO ("Finished installing internet stack");
	
	
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
 	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));  
 	
 	
 	Ipv4AddressHelper address;
 	address.SetBase ("10.1.1.0", "255.255.255.0");
	
	
	NS_LOG_INFO ("Creating connections");
	FILE *fp = fopen ("graph500","r");
	uint32_t n1, n2, n3;
	
	char ch;
	if (fp == NULL)
	{
		std::cout<<"File Open Error !!";
		perror ("Error opening file");
	}
	else
	{
		while ( ! feof (fp) )
		{
			if ( fscanf(fp,"%u%u%u%c",&n1,&n2,&n3,&ch) > 0 )
			{
				std::cout<<n1<<" - "<<n2<<"\n";
				
				NodeContainer links = NodeContainer(nodes.Get(n1),nodes.Get(n2));
				NetDeviceContainer device = p2p.Install(links);
				address.Assign(device);
				address.NewNetwork ();
			}
		}
		fclose (fp);
	}
	NS_LOG_INFO ("Finished creating connections");
	
	NS_LOG_INFO ("Populating routing tables");
 	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 	NS_LOG_INFO ("Finished populating routing tables");
	
  	 	
 	
 	 	
 	
 	/*UdpEchoServerHelper echoServer (9);

	ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (20.0));
	*/
	NS_LOG_INFO ("Installing Server Application");
	Ptr<Server_App> app = CreateObject<Server_App> ();
	app->m_port=10;
	//app->Setup (ns3TcpSocket, sinkAddress, 1040, 1000, DataRate ("1Mbps"));
	app->server_node = nodes.Get(1);
	nodes.Get (1)->AddApplication (app);
	app->SetStartTime (Seconds (1.));
	app->SetStopTime (Seconds (20.));
	NS_LOG_INFO ("Finished installing Server Application");
	
	Ptr<Node> n = nodes.Get (1);
	Ptr<Ipv4> ipv42 = n->GetObject<Ipv4> ();
      	Ipv4InterfaceAddress ipv4_int_addr = ipv42->GetAddress (1, 0);
        Ipv4Address ip_addr = ipv4_int_addr.GetLocal ();
	
	
	
	std::vector<Ptr<Client_App> > app_array; 
	
	
	NS_LOG_INFO ("Installing Client Applications");
	
	for(uint32_t k=1;k<num_nodes;k++)
	{
		Ptr<Client_App> app1 = CreateObject<Client_App> ();
		app1->m_size=1024;
		app1->m_count=1;
		app1->m_peerAddress=ip_addr;
		app1->m_peerPort=10;
		app1->hops=3;
		nodes.Get(k)->AddApplication(app1);
		app1->SetStartTime(Seconds(1.));
		app1->SetStopTime(Seconds(20.));
	}
	NS_LOG_INFO ("Finished installing Client Applications");
	
	NS_LOG_INFO ("Starting Simulation");
		
	Simulator::Run ();
	Simulator::Destroy ();
	
	NS_LOG_INFO ("Finished Simulation");
	return 0;
}
