{
  'targets': [
  {
    'target_name': 'rts',
    'type': 'executable',
    'mac_bundle': 1,
    'defines': [
      'DEFINE_FOO',
    ],
    'variables': {
      'component%': 'shared_library',
    },
    'include_dirs': [
      '../src',
      '../lib/glm',
      '../lib/v8/include',
      '../lib/glfw-3.0.1/include',
      '../lib/jsoncpp',
      '../lib/stb_truetype',
      '../lib/stbi/',
    ],
    'sources': [
      '../src/exec/rts-main.cpp',

      # generated with
      # ls src/rts/* | sed "s|\(.*\)|'../\1',|"
      '../src/rts/ActionWidget.cpp',
      '../src/rts/ActionWidget.h',
      '../src/rts/ActorPanelWidget.cpp',
      '../src/rts/ActorPanelWidget.h',
      '../src/rts/BorderWidget.cpp',
      '../src/rts/BorderWidget.h',
      '../src/rts/Camera.cpp',
      '../src/rts/Camera.h',
      '../src/rts/CommandWidget.cpp',
      '../src/rts/CommandWidget.h',
      '../src/rts/Controller.cpp',
      '../src/rts/Controller.h',
      '../src/rts/Curves.h',
      '../src/rts/EffectFactory.cpp',
      '../src/rts/EffectFactory.h',
      '../src/rts/EffectManager.cpp',
      '../src/rts/EffectManager.h',
      '../src/rts/Entity.h',
      '../src/rts/FontManager.cpp',
      '../src/rts/FontManager.h',
      '../src/rts/Game.cpp',
      '../src/rts/Game.h',
      '../src/rts/GameController.cpp',
      '../src/rts/GameController.h',
      '../src/rts/GameEntity.cpp',
      '../src/rts/GameEntity.h',
      '../src/rts/GameScript.cpp',
      '../src/rts/GameScript.h',
      '../src/rts/GameServer.cpp',
      '../src/rts/GameServer.h',
      '../src/rts/Graphics.cpp',
      '../src/rts/Graphics.h',
      '../src/rts/Input.cpp',
      '../src/rts/Input.h',
      '../src/rts/Lobby.cpp',
      '../src/rts/Lobby.h',
      '../src/rts/Map.cpp',
      '../src/rts/Map.h',
      '../src/rts/Matchmaker.cpp',
      '../src/rts/Matchmaker.h',
      '../src/rts/MatchmakerController.cpp',
      '../src/rts/MatchmakerController.h',
      '../src/rts/MatrixStack.cpp',
      '../src/rts/MatrixStack.h',
      '../src/rts/MinimapWidget.cpp',
      '../src/rts/MinimapWidget.h',
      '../src/rts/ModelEntity.cpp',
      '../src/rts/ModelEntity.h',
      '../src/rts/NativeUIBinding.cpp',
      '../src/rts/NativeUIBinding.h',
      '../src/rts/Player.cpp',
      '../src/rts/Player.h',
      '../src/rts/PlayerAction.h',
      '../src/rts/Renderer.cpp',
      '../src/rts/Renderer.h',
      '../src/rts/ResourceManager.cpp',
      '../src/rts/ResourceManager.h',
      '../src/rts/Shader.cpp',
      '../src/rts/Shader.h',
      '../src/rts/UI.cpp',
      '../src/rts/UI.h',
      '../src/rts/UIAction.h',
      '../src/rts/Widgets.cpp',
      '../src/rts/Widgets.h',

      '../src/common/BlockingQueue.h',
      '../src/common/Checksum.cpp',
      '../src/common/Checksum.h',
      '../src/common/Clock.cpp',
      '../src/common/Clock.h',
      '../src/common/Collision.cpp',
      '../src/common/Collision.h',
      '../src/common/Exception.cpp',
      '../src/common/Exception.h',
      '../src/common/FPSCalculator.h',
      '../src/common/Geometry.cpp',
      '../src/common/Geometry.h',
      '../src/common/Logger.cpp',
      '../src/common/Logger.h',
      '../src/common/NavMesh.cpp',
      '../src/common/NavMesh.h',
      '../src/common/NetConnection.cpp',
      '../src/common/NetConnection.h',
      '../src/common/ParamReader.cpp',
      '../src/common/ParamReader.h',
      '../src/common/Types.cpp',
      '../src/common/Types.h',
      '../src/common/WorkerThread.h',
      '../src/common/image.cpp',
      '../src/common/image.h',
      '../src/common/kissnet.cpp',
      '../src/common/kissnet.h',
      '../src/common/util.cpp',
      '../src/common/util.h',

      '../lib/jsoncpp/json/json.h',
      '../lib/jsoncpp/jsoncpp.cpp',
      '../lib/stb_truetype/stb_truetype.h',
      '../lib/stbi/stb_image.c',
    ],
    'conditions': [
      ['OS=="win"', {
        'defines': [
          '_USE_MATH_DEFINES',
          '_VARIADIC_MAX=8',
          'GLFW_DLL',
        ],
        'include_dirs': [
          '../lib/glew-1.7.0/include',
          '../lib/boost_1_50_0',
          '../lib/assimp/include',
        ],
        'link_settings': {
          'libraries': [
            'glew32',
            'opengl32',
            'glfw3dll',
            'assimp',
            'v8',
            'glu32',
          ],
        },
        'msvs_settings': {
          'VCLinkerTool': {
            'OutputFile': '../rts.exe',
            'AdditionalLibraryDirectories': [
              '../lib/assimp/lib-msvc',
              '../lib/boost_1_50_msvc_32',
              '../lib/glew-1.7.0/lib',
              '../lib/glfw-3.0.1/lib-msvc110',
              '../lib/v8/lib-msvs',
            ],
          },
        },
        'copies': [{
          'destination': '../',
          'files': [
            '../msvc/DLLs/Assimp32.dll',
            '../msvc/DLLs/glew32.dll',
            '../msvc/DLLs/glfw3.dll',
          ],
        }],
      }],
      ['OS=="mac"', {
        'defines': [
          'MACOSX',
        ],
        'include_dirs': [
          '/usr/local/include/',
        ],
        'mac_bundle_resources': [
          '../fonts/',
          '../images/',
          '../maps/',
          '../models/',
          '../jscore/',
          '../scripts/',
          '../shaders/',
          '../config.json',
          '../local.json',
          '../local.json.default',
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS': [
            '-Wall',
            '-std=c++11',
            '-stdlib=libc++',
          ],
          'OTHER_LDFLAGS': [
            '-L/usr/local/lib',
            '-stdlib=libc++',
            '-lGLEW',
            '-lassimp',
            '-lboost_system',
            '-lboost_filesystem',
            '../lib/v8/lib-macosx/libv8_base.x64.a',
            '../lib/v8/lib-macosx/libv8_snapshot.a',
            '../lib/glfw-3.0.1/lib-macosx/libglfw3.a',
            '-framework Cocoa',
            '-framework OpenGL',
            '-framework IOKit',
          ],
        },
      }],
    ],
  },
  ],
}
