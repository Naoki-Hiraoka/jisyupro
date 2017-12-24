似たような現象が起こりました。
特定の条件でfuncallすると

エラーにならない例
```
(print "1")

(setq func1 #'(lambda (x) (setq *a* 1)))
(funcall func1 1)

(print "2")

(defun func2 (x) (setq *a* x))
(setq function #'func2)
(funcall function 1)

(print "3")
```
結果
```
mech-user@test1-pc:~/catkin_ws/src/jisyupro/memo$ roseus ./temp8.l
configuring by "/opt/ros/kinetic/share/euslisp/jskeus/eus//lib/eusrt.l"
;; readmacro ;; object ;; packsym ;; common ;; constants ;; stream ;; string ;; loader ;; pprint ;; process ;; hashtab ;; array ;; mathtran ;; eusdebug ;; eusforeign ;; coordinates ;; tty ;; history ;; toplevel ;; trans ;; comp ;; builtins ;; par ;; intersection ;; geoclasses ;; geopack ;; geobody ;; primt ;; compose ;; polygon ;; viewing ;; viewport ;; viewsurface ;; hid ;; shadow ;; bodyrel ;; dda ;; helpsub ;; eushelp ;; xforeign ;; Xdecl ;; Xgraphics ;; Xcolor ;; Xeus ;; Xevent ;; Xpanel ;; Xitem ;; Xtext ;; Xmenu ;; Xscroll ;; Xcanvas ;; Xtop ;; Xapplwin 
connected to Xserver DISPLAY=:0
X events are being asynchronously monitored.
;; pixword ;; RGBHLS ;; convolve ;; piximage ;; pbmfile ;; image_correlation ;; oglforeign ;; gldecl ;; glconst ;; glforeign ;; gluconst ;; gluforeign ;; glxconst ;; glxforeign ;; eglforeign ;; eglfunc ;; glutil ;; gltexture ;; glprim ;; gleus ;; glview ;; toiv-undefined ;; fstringdouble irtmath irtutil irtc irtgeoc irtgraph pgsql irtgeo euspqp pqp irtscene irtmodel irtdyna irtrobot irtsensor irtbvh irtcollada irtpointcloud irtx eusjpeg euspng png irtimage irtglrgb 
;; extending gcstack 0x4dea9d0[16374] --> 0x5265760[32748] top=3d1c
irtgl irtglc irtviewer 
EusLisp 9.23( 1.1.0) for Linux64 created on ip-172-30-1-103(Sat Nov 4 18:28:33 PST 2017)
roseus ;; loading roseus("1.6.3") on euslisp((9.23 ip-172-30-1-103 Sat Nov 4 18:28:33 PST 2017  1.1.0))
eustf roseus_c_util "1"
"2"
"3"
1.irteusgl$ 

```
