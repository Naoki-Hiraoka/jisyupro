(ros::load-ros-manifest "hiraoka_semi")
(ros::roseus "fibonacci_client")
(setq *c* (instance ros::simple-action-client :init
		    "fibonacci" hiraoka_semi::background_subtractor2Action)))
(send *c* :wait-for-server)
(setq goal (instance hiraoka_semi::background_subtractor2ActionGoal :init))
(send goal :goal :frames 10)
(send *c* :send-goal goal)
(send *c* :wait-for-result)
