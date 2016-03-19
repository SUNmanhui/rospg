#include <ros/ros.h>

#include <std_msgs/String.h>
#include <rospg/TopologicalNavigationMap.h>
#include <rospg/TopoNavEdgeMsg.h>
#include <rospg/TopoNavNodeMsg.h>


//#include <std_msgs/int.h>
#include <pqxx/pqxx>
//#include <boost/foreach.hpp>
#include <iostream>
#include <sstream>
#include <math.h>

class Pgsql
{
	public:
		Pgsql(int argc, char**argv);
		~Pgsql();		
	private:
        	ros::NodeHandle n;
		ros::Publisher pub1;
		ros::Subscriber sub1;
		
			
		void normalizeMsg();
		
};


