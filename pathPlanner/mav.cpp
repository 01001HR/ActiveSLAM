#include "mav.h"
#include "ros/ros.h"



mav::mav()
{
    visualize=new display();
    plan =new actuator();
    timer = nh_.createTimer(ros::Duration(0.5), &mav::timerCallback,this);
    map_search=new octomap_search();
    planner_talk=nh_.advertise<std_msgs::String>("jaistquad/motionplan",1);
    service = nh_.advertiseService("motionplan",&mav::talk,this);
    srv_obs = nh_.advertiseService("obstacle",&mav::collision,this);
    robot_srv=nh_.serviceClient<active_slam::obstacle>("localization");
}

mav::~mav()
{

}



void mav::run(){
        ROS_INFO("SENSOR SUBSCRIBTION ENABLE vs 2.00");
        sleep(1);
        map_search->debugger("path planner vs 2.00 is intialized");

      //ORB_SLAM DEPENDECIES
        orbTraker=nh_.subscribe("ORB_SLAM/Debug",10,&mav::takerCallback, this);
        cloud_sub=nh_.subscribe("/ORB_SLAM/PointCloud_raw",10,&mav::pointcloudCallback, this);

        //OCTOMAP DEPENDENCIES
        projectedmap_sub=nh_.subscribe("/projected_map",10,&mav::projectedmapCallback, this);

        //RVIZ DEPENDECIES
        rviz_goal=nh_.subscribe("move_base_simple/goal",10,&mav::goalCallback, this);
        rviz_pose=nh_.subscribe("initialpose",10,&mav::poseCallback, this);



}

 void mav::timerCallback(const ros::TimerEvent& e){
     mutex.lock();

     mutex.unlock();

 }

 //user input
 bool mav::talk(active_slam::plannertalk::Request  &req,
          active_slam::plannertalk::Response &res)
 {
    if(req.option==1){
        map_search->debugger("p2p planner motion is selected");
        p2pNav=true;}
    else if(req.option==2){
        p2pNav=false;
        map_search->debugger("traj_2 folower motion is selected");
    }
    res.result=true;
     return true;
 }


 bool mav::collision(active_slam::obstacle::Request  &req,
          active_slam::obstacle::Response &res)
 {
//       ROS_INFO("obs_serv");
    if(req.id==1){
     plan->updateCloudScale(req.state_x, req.state_y);
     stringstream ss;
     ss<< "scale " <<req.state_x << " alt "<< req.state_y;

      map_search->debugger(ss.str());
    ScaleFix=true;
    }
    else{
     double A[2];
     map_search->obstacle_avoid(A,req.state_x,req.state_y);
     res.x=A[0];//left or right
     res.y=A[1];//forward or backward
    }
     return true;

 }

 //ORB_SLAM DEPENDECIES
void mav::pointcloudCallback(const boost::shared_ptr<const sensor_msgs::PointCloud2>& input)
{
//        mapPublisher
    if(!ScaleFix)return;
    mutex.lock();
        plan->pointcloudCallback(input);
    mutex.unlock();

}

void mav::takerCallback(const std_msgs::String &msg){

}

void mav::poseCallback(const geometry_msgs::PoseWithCovarianceStampedConstPtr  msg){


    map_search->updateRobot(msg->pose.pose.position.x,msg->pose.pose.position.y);
   ROS_INFO("pose (%f, %f) ",msg->pose.pose.position.x,msg->pose.pose.position.y);
}


void mav::goalCallback(const geometry_msgs::PoseStampedConstPtr msg){
    ROS_INFO_STREAM("Goal received");
    if(p2pNav){
        active_slam::obstacle robo;
        robo.request.id= robo.request.state_x= robo.request.state_y=1;
        if (robot_srv.call(robo)){
            ROS_INFO("Current Robot (%f, %f)",robo.response.x,robo.response.y);

             map_search->updateRobot(robo.response.x,robo.response.y);

            map_search->goalPublisher(msg->pose.position.x,msg->pose.position.y);
            stringstream ss;
            ss<< "robot ("<< robo.response.x<< ", "<< robo.response.y<<") ";
            ss<< "Goal ("<< msg->pose.position.x<< ", "<< msg->pose.position.y<<") ";
            map_search->debugger(ss.str());
        }
         else
                map_search->debugger("Error to retrive new goal");
    }
    else{
        active_slam::obstacle robo;
        robo.request.id= robo.request.state_x= robo.request.state_y=1;
        if (robot_srv.call(robo)){
            ROS_INFO("Current Robot (%f, %f)",robo.response.x,robo.response.y);

             map_search->updateRobot(robo.response.x,robo.response.y);
            stringstream ss;
            ss<< "robot ("<< robo.response.x<< ", "<< robo.response.y<<") ";
            ss<< "Goal ("<< msg->pose.position.x<< ", "<< msg->pose.position.y<<") ";
            map_search->debugger(ss.str());
        }
         else
                map_search->debugger("Error to retrive new goal");


        vector<float>robot;
        robot.push_back(msg->pose.position.x);
        robot.push_back(msg->pose.position.y);
        map_search->find_status();
        map_search->foundPath(robot);
    }


}



    //OCTOMAP DEPENDENCIES
void mav::projectedmapCallback(const nav_msgs::OccupancyGridPtr msg){
    plan->writemap(msg);

}
