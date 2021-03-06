(ros::load-ros-manifest "roseus")
(ros::load-ros-manifest "std_msgs")

(setq *button1* nil)
(setq *button2* nil)
(setq *button3* nil)
(defun get-button (num)
  (case num
    (1 (prog1 *button1*
	 (setq *button1* nil)))
    (2 (prog1 *button2*
	 (setq *button2* nil)))
    (3 (prog1 *button3*
	 (setq *button3* nil)))
    )
  )
(defun button-cb (msg)
  (case (send msg :data)
    (1 (setq *button1* t) (print *button1*))
    (2 (setq *button2* t))
    (3 (setq *button3* t))
    )
  )
(ros::subscribe "/main/button" std_msgs::Int32 #'button-cb)
