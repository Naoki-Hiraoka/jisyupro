(defclass tempclass1
  :super ros::object
  :slots (function))
(defmethod tempclass1
  (:init (x)
	 (setq function x))
  (:cb (x)
       (print function)
       (funcall function x)))

(defun func (x) (setq *a* 1))
(setq test (instance tempclass1 :init #'func))

(print "(send b :cb 1)")
(send test :cb 1)

;; mech-user@test1-pc:~/catkin_ws/src/jisyupro/memo$ roseus ./temp6.l
;; configuring by "/opt/ros/kinetic/share/euslisp/jskeus/eus//lib/eusrt.l"
;; ;; readmacro ;; object ;; packsym ;; common ;; constants ;; stream ;; string ;; loader ;; pprint ;; process ;; hashtab ;; array ;; mathtran ;; eusdebug ;; eusforeign ;; coordinates ;; tty ;; history ;; toplevel ;; trans ;; comp ;; builtins ;; par ;; intersection ;; geoclasses ;; geopack ;; geobody ;; primt ;; compose ;; polygon ;; viewing ;; viewport ;; viewsurface ;; hid ;; shadow ;; bodyrel ;; dda ;; helpsub ;; eushelp ;; xforeign ;; Xdecl ;; Xgraphics ;; Xcolor ;; Xeus ;; Xevent ;; Xpanel ;; Xitem ;; Xtext ;; Xmenu ;; Xscroll ;; Xcanvas ;; Xtop ;; Xapplwin 
;; connected to Xserver DISPLAY=:0
;; X events are being asynchronously monitored.
;; ;; pixword ;; RGBHLS ;; convolve ;; piximage ;; pbmfile ;; image_correlation ;; oglforeign ;; gldecl ;; glconst ;; glforeign ;; gluconst ;; gluforeign ;; glxconst ;; glxforeign ;; eglforeign ;; eglfunc ;; glutil ;; gltexture ;; glprim ;; gleus ;; glview ;; toiv-undefined ;; fstringdouble irtmath irtutil irtc irtgeoc irtgraph pgsql irtgeo euspqp pqp irtscene irtmodel irtdyna irtrobot irtsensor irtbvh irtcollada irtpointcloud irtx eusjpeg euspng png irtimage irtglrgb 
;; ;; extending gcstack 0x47e09d0[16374] --> 0x4c5b760[32748] top=3d1c
;; irtgl irtglc irtviewer 
;; EusLisp 9.23( 1.1.0) for Linux64 created on ip-172-30-1-103(Sat Nov 4 18:28:33 PST 2017)
;; roseus ;; loading roseus("1.6.3") on euslisp((9.23 ip-172-30-1-103 Sat Nov 4 18:28:33 PST 2017  1.1.0))
;; eustf roseus_c_util (lambda-closure func 7889200 0 (x) (setq *a* 1))
;; ;; Segmentation Fault.
;; ;; in (setq *a* 1)
;; ;; You are still in a signal handler.
;; ;;Try reset or throw to upper level as soon as possible.
;; ;; code=1914362288 x=721ad880 addr=4
;; Fatal: 
