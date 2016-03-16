/*
 * Copyright [2016] <Bonn-Rhein-Sieg University>
 *
 * Author: Oscar Lima (olima_84@yahoo.com)
 * 
 * Node used to call external scripts on your local pc
 * 
 */

#ifndef RUN_SCRIPT_NODE_H
#define RUN_SCRIPT_NODE_H

#include <ros/ros.h>
#include <ros/package.h>
#include <std_msgs/String.h>
#include <run_script/run_script.h>
#include <string>
#include <vector>

class RunScriptNode
{
    public:
        RunScriptNode();
        ~RunScriptNode();

        // variable initialization function
        void init();

        // callback for event_in received msg
        void runScriptCallBack(const std_msgs::String::ConstPtr& msg);

        // get parameters from param server
        void getParams();

        // ros node main loop
        void update();

    private:
        // flag used to know when we have received a callback
        bool callback_received_;

        // ros related variables
        ros::NodeHandle nh_;
        ros::Publisher pub_event_in_;
        ros::Subscriber sub_event_in_;

        // for receiving event in msg
        std_msgs::String event_in_msg_;

        // for publishing event_out string msg
        std_msgs::String even_out_msg_;

        // for storing the arguments that will be read from param server
        std::vector<std::string> script_arguments_;

        // to store the path of the script
        std::string full_path_to_script_;

        // flag that indicates of script arguments are available
        bool are_args_available_;

        // generic class to call external scripts
        RunScript script_handler_;
};
#endif  // RUN_SCRIPT_NODE_H
