#include <iostream>
#include <stdio.h>
#include <string.h>
#include "geometry_msgs/Twist.h"

#include "ros/ros.h"
#include "eklavya4_roboteq/SetSpeed.h"

#include "eklavya4_roboteq/RoboteqDevice.h"
#include "eklavya4_roboteq/ErrorCodes.h"
#include "eklavya4_roboteq/Constants.h"

using namespace std;
#define r 10   //the radius of wheel in centimetre
geometry_msgs::Twist vel_msg;
ros::Publisher controller_pub;
RoboteqDevice device;
int status = 0;

bool setSpeed(eklavya4_roboteq::SetSpeed::Request &req, eklavya4_roboteq::SetSpeed::Response &res) {
    res.code = 0;
    
	if((status = device.SetCommand(_GO, 1, req.v_r)) != RQ_SUCCESS) {
		ROS_INFO("Failed... Error code --> %d", status);
	}
	else {
		ROS_DEBUG("Succeeded.");
	}
	
	if((status = device.SetCommand(_GO, 2, (-1)*req.v_l)) != RQ_SUCCESS) {   // multiplied with -1 as the motor rotates in reverse direction due to connection
		ROS_INFO("Failed... Error code --> %d", status);
	}
	else {
		ROS_DEBUG("Succeeded.");
	}
	/************************FEEDBACK*****************************/
	int left_speed=0,right_speed=0;
	if((status = device.GetValue(_ABSPEED, 1, left_speed)) != RQ_SUCCESS) {
		ROS_INFO("Failed... Error code --> ", status);
	}
	else {
		ROS_DEBUG("Succeeded.");
	}
		
	usleep(100);
	
	if (status == 0) {
		
		if((status = device.GetValue(_ABSPEED, 2, right_speed)) != RQ_SUCCESS) {
			ROS_INFO("Failed... Error code --> ", status);
		}
		else {
			ROS_DEBUG("Succeeded.");
		}
	}
	vel_msg.linear.x=(left_speed*2*3.1415*r)/60;             //To convert velocity in cm/s from RPM
	vel_msg.linear.y=(right_speed*2*3.1415*r)/60;
	/****************************FEEDBACK**************************/
	usleep(100);
	
	int numberOfattempts = 0;
	
	while (status != 0 && numberOfattempts < 10) {
		
		usleep(500000);
		ROS_INFO("Attempting server restart...");
		device.Disconnect();
		status = device.Connect("/dev/serial/by-id/usb-Roboteq_Motor_Controller_498954A73235-if00"); //usb-Roboteq_Motor_Controller_498954A73235-if00 for old roboteq
		if (status == 0) {                                                                           //usb-Roboteq_Motor_Controller_8D7314985754-if00 for auro roboteq
			ROS_INFO("Connection re-established...");
		}
		numberOfattempts++;
		
	}
	
	if (numberOfattempts == 10 && status != 0) {
		ROS_ERROR("Could not connect to Roboteq Motor Controller... Aborting operation...");
		res.code = -1;
	}
	
    ROS_INFO("sending back response: [%ld]", (long int)res.code);
    
    return true;
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "motor_controller_server");
    ros::NodeHandle n;
    
    string response = "";
    
    ROS_INFO("\n\n--- Roboteq Motor Controller Request Gateway Server ---\n");
    ROS_INFO("Initializing...");
	usleep(500000);

	status = device.Connect("/dev/serial/by-id/usb-Roboteq_Motor_Controller_498954A73235-if00"); //usb-Roboteq_Motor_Controller_498954A73235-if00 for old roboteq
	//status = device.Connect("/dev/ttyACM0");                                                   //usb-Roboteq_Motor_Controller_8D7314985754-if00 for auro roboteq

	while (status != RQ_SUCCESS && ros::ok())
	{
		ROS_INFO("Error connecting to device: %d", status);
		ROS_INFO("Attempting server restart...");
		usleep(999999);
		device.Disconnect();
		status = device.Connect("/dev/serial/by-id/usb-Roboteq_Motor_Controller_498954A73235-if00"); //usb-Roboteq_Motor_Controller_498954A73235-if00 for old roboteq
		if (status == 0) {                                                                           //usb-Roboteq_Motor_Controller_8D7314985754-if00 for auro roboteq
			ROS_INFO("Connection re-established...");
		}
	}
  
    ros::ServiceServer service1 = n.advertiseService("motor_controller", setSpeed);
    controller_pub = n.advertise<geometry_msgs::Twist>("result_vel",1000);
    //ros::ServiceServer service2 = n.advertiseService("motor_speed", getSpeed);
    ROS_INFO("Server initialized...");
    ROS_INFO("Ready to control motors...");
    ros::Rate loop_rate(10.0);
	
	vel_msg.linear.x=0;
	vel_msg.angular.z=0;
	vel_msg.linear.y=0;
	vel_msg.linear.z=0;
	vel_msg.angular.x=0;
	vel_msg.angular.y=0;
      
	controller_pub.publish(vel_msg);
	
	loop_rate.sleep();
	
	while(ros::ok())
	{
	controller_pub.publish(vel_msg);
	ros::spinOnce();
	ROS_INFO("%f", vel_msg.linear.x);
	}    
    //ros::spin();
	
	device.Disconnect();
  
    return 0;
}