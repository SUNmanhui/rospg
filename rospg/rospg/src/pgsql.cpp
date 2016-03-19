#include "pgsql.h"
#include <pqxx/pqxx>
#include <iostream>
#include "stdafx.h"

using namespace std;
using namespace pqxx;

Pgsql::Pgsql(int argc, char**argv)
{
	double frequency;

	n.param("main_loop_frequency", frequency, double(4.0));

	ros::Rate r(frequency);

	pub1 = n.advertise<rospg::TopologicalNavigationMap>("gistopomap",1,true);

	//toponav_map_topic = "Pgsql/msg/topological_navigation_map";
   // ROS_INFO("Waiting for the toponavmap at topic: '%s'", toponav_map_topic.c_str());
  	//sub1 = n.subscribe(toponav_map_topic, 1, &Pgsql::normalizeMsg, this);
	normalizeMsg();	
}

Pgsql::~Pgsql()
{

}

void Pgsql::normalizeMsg(){
	
	ROS_INFO("getShortestPath start!");

	std::stringstream ss;


	//connect to the database	
	connection C("dbname=postgres user=postgres password=123456 \
      hostaddr=127.0.0.1 port=5432");
    if (C.is_open()) {
       ROS_INFO("Opened database successfully: %s",C.dbname());
    } else {
       //cout << "Can't open database" << endl;
       ROS_INFO("Can't open database!");
    }

    char *sql;
   
    /* Create SQL statement to ADD column that needed */
    sql = "ALTER TABLE roadkeda ADD COLUMN source integer;ALTER TABLE roadkeda ADD COLUMN target integer; ALTER TABLE roadkeda ADD COLUMN length double precision;";

    /* Create a transactional object. */
    work W(C);
    /* Execute SQL query */
    W.exec( sql );
    //W.commit();

    /* Create SQL statement to create topology */
    sql = "SELECT pgr_createTopology('roadkeda',0.001, 'geom', 'gid');";
    W.exec( sql );
    // W.commit();

    /* Create SQL statement to add index */
    sql = "CREATE INDEX source_idx2 ON roadkeda(source);CREATE INDEX target_idx2 ON roadkeda(target);update roadkeda set length =st_length(geom);";
    W.exec( sql );
    //W.commit();

    /* Create SQL statement to add costy */
    sql = "ALTER TABLE roadkeda ADD COLUMN reverse_cost double precision;UPDATE roadkeda SET reverse_cost =length;";
    W.exec( sql );
    //W.commit();

    /* Create SQL statement to get shortestpath by dijkstra */
    sql = "SELECT seq, id1 AS node, id2 AS edge, cost,geom into path_res FROM pgr_dijkstra('SELECT gid AS id,source::integer,target::integer,length::double precision AS cost FROM roadkeda',30, 60, false, false) as di join roadkeda pt on di.id2 = pt.gid;";
    W.exec( sql );
    //W.commit();

    /* Create SQL statement to get result */
    sql = "SELECT node,cost FROM path_res;";
    pqxx::result r = W.exec( sql );
    //W.commit();


	// for debug
	ROS_DEBUG("prepare TopoNavMapMsg");

    // declare a msg
	rospg::TopologicalNavigationMap msg_map;

	// set the stamp
  	msg_map.header.stamp = ros::Time::now();
	msg_map.header.frame_id = "toponav_map";
    
    // set other parameters
	int count = 0;
	string data_node0, data_node1;
	string data_cost0, data_cost1;
    for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row)
    {
		int msg_id = 0;
		for (pqxx::tuple::const_iterator field = row->begin(); field != row->end(); ++field){
			switch(msg_id){
				case 0:	data_node1 = field->c_str();break;                     
			    case 1:	data_cost1 = field->c_str();break;                     
			    default: break;
			}
			msg_id++;
		}
        if(count==0){			 
        	data_node0 = data_node1;
			data_cost0 = data_cost1;
			count =1;
			continue;    
	    }
		
        // current  data are saved in data_node1, data_cost1
        // previous data are saved in data_node0, data_cost0
 		// define a new edge
			rospg::TopoNavEdgeMsg msg_edge;
			// 
			msg_edge.edge_id = count-1;
			msg_edge.last_updated = ros::Time::now();
			//msg_edge.start_node_id = atoi(const_cast<const char*>(data_node0));
			msg_edge.start_node_id = atoi((data_node0.c_str()));			
			msg_edge.type = 1;
			//msg_edge.end_node_id = atoi(const_cast<const char*>(data_node1));
			msg_edge.end_node_id = atoi(data_node1.c_str());
			msg_edge.cost = atof(data_cost0.c_str());
		// define a new node
			rospg::TopoNavNodeMsg msg_node;
		
			msg_node.node_id = atoi(data_node0.c_str());
			msg_node.last_updated = ros::Time::now();
  			msg_node.last_pose_updated = ros::Time::now();
  			msg_node.area_id = 0;
  			msg_node.pose.orientation.w = 1.0;
  			msg_node.is_door = false;

		// cache
   	    data_node0 = data_node1;
		data_cost0 = data_cost1;
		// push back and deal with next data
		msg_map.edges.push_back(msg_edge);	
		msg_map.nodes.push_back(msg_node);	    
		count++;
    }
	  

	W.commit();

	
	//cout << "Table created successfully" << endl;
	C.disconnect ();

	

}



