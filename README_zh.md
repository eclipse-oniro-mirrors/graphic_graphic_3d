# AGP引擎

- 简介
- 目录
- 相关仓

## 简介
AGP引擎是华为自研3D渲染引擎，3D引擎是操作系统Graphic模块的重要组成部分，对下屏蔽不同GPU硬件驱动、不同渲染后端的差异，对上提供基础渲染API，提供3D渲染能力。AGP渲染引擎作为一种软件框架，主要功能是创建、渲染和管理3D世界，插件系统、渲染组件、GPU资源管理组件、ECS组件系统等模块都是构成AGP引擎的关键部分。基于APG引擎的3D控件是ArKUI的基础组件，其提供了加载模型、动画播放、自定义灯光、相机以及纹理等能力，供开发者自定义3D模型。
## 目录

```
foundation/graphic/graphic_3d/
├── 3d_widget_adapter                  # 与ArkUI接口代码
├── lume                               # 引擎核心代码
│   ├── Lume_3D                        # 引擎3D渲染代码
│   ├── LumeBase                       # 引擎基础库代码
│   ├── LumeEngine                     # 引擎加载插件、资源管理代码
│   └── LumeRender                     # 引擎渲染后端代码
```

## 相关仓