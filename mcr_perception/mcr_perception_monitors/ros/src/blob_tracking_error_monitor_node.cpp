#include <mcr_perception_monitors/blob_tracking_error_monitor_node.h>

BlobTrackingErrorMonitorNode::BlobTrackingErrorMonitorNode(ros::NodeHandle &nh) : node_handler_(nh)
{
    dynamic_reconfig_server_.setCallback(boost::bind(&BlobTrackingErrorMonitorNode::dynamicReconfigCallback, this, _1, _2));
    event_sub_ = node_handler_.subscribe("event_in", 1, &BlobTrackingErrorMonitorNode::eventCallback, this);
    error_sub_ = node_handler_.subscribe("pose_error", 1, &BlobTrackingErrorMonitorNode::errorCallback, this);
    event_pub_ = node_handler_.advertise<std_msgs::String>("event_out", 1);
    run_state_ = INIT;
    start_tracking_error_monitor_ = false;
    tracking_error_sub_status_ = false;  
}

BlobTrackingErrorMonitorNode::~BlobTrackingErrorMonitorNode()
{
    event_sub_.shutdown();
    error_sub_.shutdown();
    event_pub_.shutdown();  
}

void BlobTrackingErrorMonitorNode::dynamicReconfigCallback(mcr_perception_monitors::BlobTrackingErrorMonitorConfig &config, uint32_t level){
    threshold_linear_x_ = config.threshold_linear_x;
    threshold_linear_y_ = config.threshold_linear_y;
    threshold_linear_z_ = config.threshold_linear_z;
    threshold_angular_x_ = config.threshold_angular_x;
    threshold_angular_y_ = config.threshold_angular_y;
    threshold_angular_z_ = config.threshold_angular_z;
}

void BlobTrackingErrorMonitorNode::eventCallback(const std_msgs::String &event_command)
{
    if (event_command.data == "e_start") {
        start_tracking_error_monitor_ = true;
        ROS_INFO("Blob Tracking Error Monitor ENABLED");
    } else if (event_command.data == "e_stop") {
        start_tracking_error_monitor_ = false;
        ROS_INFO("Blob Tracking Error Monitor DISABLED");
    }
}

void BlobTrackingErrorMonitorNode::errorCallback(const mcr_manipulation_msgs::ComponentWiseCartesianDifference::ConstPtr &pose_eror)
{
    error_ = *pose_eror;
    tracking_error_sub_status_ = true;
}

void BlobTrackingErrorMonitorNode::states()
{
    switch (run_state_) {
        case INIT:
            initState();
            break;
        case IDLE:
            idleState();
            break;
        case RUNNING:
            runState();
            break;
        default:
            initState();
    }
}

void BlobTrackingErrorMonitorNode::initState()
{
    if (tracking_error_sub_status_) {
        run_state_ = IDLE;
        tracking_error_sub_status_ = false;
    }
}

void BlobTrackingErrorMonitorNode::idleState()
{
    if (start_tracking_error_monitor_) {
        run_state_ = RUNNING;
    } else {
        run_state_ = INIT;
    }
}

void BlobTrackingErrorMonitorNode::runState()
{
    blobTrackingErrorMonitor();
    run_state_ = INIT;
}

void BlobTrackingErrorMonitorNode::blobTrackingErrorMonitor()
{

    
    if( (fabs(error_.linear.x)< threshold_linear_x_) && (fabs(error_.linear.y)< threshold_linear_y_) && (fabs(error_.linear.z)< threshold_linear_z_)){
        if( (fabs(error_.angular.x)< threshold_angular_x_) && (fabs(error_.angular.y)< threshold_angular_y_) && (fabs(error_.angular.z)< threshold_angular_z_)){
            status_msg_.data = "e_done";
            event_pub_.publish(status_msg_);
        }
    }
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "blob_tracking_error_monitor");
    ros::NodeHandle nh("~");
    ROS_INFO("Blob Tracking Error Monitor Node Initialised");
    BlobTrackingErrorMonitorNode btem(nh);
    while (ros::ok()) {
        btem.states();
        ros::spinOnce();
    }
    return 0;
}