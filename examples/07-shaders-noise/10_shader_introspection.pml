;; ========================================
;; Shader 内省与调试：shader-uniforms, shader-validate, eval-shader
;; ========================================
;; 运行: pml.exe 10_shader_introspection.pml

(set-backend! "skia")
(canvas 200 200 :bg "#1a1a2e")

;; 编译一个有多个 uniform 的 SkSL 着色器
(define blur-shader (shader "
  uniform float u_bl;
  uniform float u_br;
  uniform float u_tl;
  uniform float u_tr;
  uniform float u_seed;

  half4 main(float2 xy) {
    return half4(
      xy.x / 200.0,
      xy.y / 200.0,
      (xy.x * xy.y) / 40000.0,
      1.0
    );
  }
"))

;; ---- 3a: shader-uniforms — 内省 uniform 声明 ----
(print "=== shader-uniforms ===")
(define uniforms (shader-uniforms blur-shader))
(println "Uniforms:" uniforms)
;; → ((u_bl float 0 4) (u_br float 4 4) (u_tl float 8 4) (u_tr float 12 4) (u_seed float 16 4))

(dotimes (i (length uniforms))
  (let ((u (nth i uniforms)))
    (println "  uniform #" i ": " (car u) " " (cadr u))))

;; ---- 3b: shader-validate — 预检查 uniform 数据 ----
(print "=== shader-validate ===")
(define ok-data (make-uniforms 0.1 0.2 0.3 0.4 0.5))
(println "Valid data ok? " (shader-validate blur-shader ok-data))  ;; → :ok

;; ---- 4b: eval-shader — 采样像素颜色 ----
(print "=== eval-shader ===")
(define applied (apply-uniforms blur-shader ok-data))
(println "eval at (50,50):"   (eval-shader applied 50 50))
(println "eval at (100,100):" (eval-shader applied 100 100))
(println "eval at (200,200):" (eval-shader applied 200 200))

;; ---- 可视化 ----
(add (apply-shader! (rect 0 0 200 200) applied))
(render "10_shader_introspection.png")
(println "Done! Check 10_shader_introspection.png")
