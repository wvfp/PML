; 示例 3：资源路径与缓存
; 演示 asset-path? 检查路径、load-image 别名、clear-assets! 清空缓存。

(set-backend! "skia")
(canvas 400 200 :bg "#1A1A2E")

(println "grass exists? " (asset-path? "../assets/grass.png"))
(println "missing exists? " (asset-path? "../assets/missing.png"))

; load-image 是 image 的别名
(define hero (load-image "../assets/character.png"
                         :x 170 :y 60
                         :width 64 :height 64))
(add hero)

; 清空资源缓存（下一次 image 会重新加载文件）
(clear-assets!)
(println "assets cleared")

(add (text 200 30 "Asset Path & Cache" :fill "#FFFFFF" :font-size 18 :align 'center))

(render "03_asset_path_and_cache.png")
