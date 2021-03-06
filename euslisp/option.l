(defmethod dxl-armed-turtlebot-interface
  (:start-grasp2
   (&optional (arm :arm) &key ((:gain g) 0.5) ((:objects objs) objects))
   "Start grasp mode."
   (send self :set-compliance-slope 7 1023)
   (send self :set-torque-limit 7 g)
   (send robot :gripper arm :angle-vector
         (send-all (send robot :gripper arm :joint-list) :min-angle))
   (send self :angle-vector (send robot :angle-vector) 1000 :gripper-controller)
   (send self :wait-interpolation :gripper-controller)
   (send self :state)
   (send robot :gripper arm :angle-vector
         (mapcar #'(lambda (x) (- x 5)) (send-all (send robot :gripper arm :joint-list) :joint-angle))) ;; 5[deg]
   (send self :angle-vector (send robot :angle-vector) 200 :gripper-controller)
   ;;以下の1行
   (send self :set-compliance-slope 7 100)
   (send self :wait-interpolation :gripper-controller)
   )

  )
  start-grasp ()
  (send *ri* :start-grasp :gain 1.0));;gainがdefaultでは0.5
