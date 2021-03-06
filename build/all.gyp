{
  'includes': ['variables.gypi'],
  'targets': [
  {
    'target_name': 'libcommon',
    'type': 'static_library',
    'include_dirs': [
      '../src',
      '../lib/glm',
      '../lib/jsoncpp',
    ],
    'sources': [
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
    ],
  },
  {
    'target_name': 'libengine',
    'type': 'static_library',
    'include_dirs': [
      '../lib/glew-1.7.0/include',
      '../lib/glfw-3.0.1/include',
      '../lib/stb_truetype/',
      '../lib/stbi/',
    ],
    'sources': [
      '../src/rts/Camera.cpp',
      '../src/rts/Camera.h',
      '../src/rts/Controller.cpp',
      '../src/rts/Controller.h',
      '../src/rts/Curves.h',
      '../src/rts/EffectManager.cpp',
      '../src/rts/EffectManager.h',
      '../src/rts/FontManager.cpp',
      '../src/rts/FontManager.h',
      '../src/rts/Graphics.cpp',
      '../src/rts/Graphics.h',
      '../src/rts/Input.cpp',
      '../src/rts/Input.h',
      '../src/rts/MatrixStack.cpp',
      '../src/rts/MatrixStack.h',
      '../src/rts/ModelEntity.cpp',
      '../src/rts/ModelEntity.h',
      '../src/rts/Renderer.cpp',
      '../src/rts/Renderer.h',
      '../src/rts/ResourceManager.cpp',
      '../src/rts/ResourceManager.h',
      '../src/rts/Shader.cpp',
      '../src/rts/Shader.h',
      '../src/rts/UI.cpp',
      '../src/rts/UI.h',
      '../src/rts/Widgets.cpp',
      '../src/rts/Widgets.h',
      '../lib/stbi/stb_image.c',
    ],
    'conditions': [
      ['OS=="win"', {
        'include_dirs': [
          '../lib/glew-1.7.0/include',
          '../lib/assimp/include',
        ],
      }],
    ],
  },
  {
    'target_name': 'libgame-core',
    'type': 'static_library',
    'dependencies': [
      'libcommon',
    ],
    'include_dirs': [
      '../lib/v8/include',
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        '../lib/v8/include',
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'OTHER_LDFLAGS': [
              '-lboost_filesystem',
              '-lboost_system',
              '../lib/v8/lib-macosx/libv8_base.x64.a',
              '../lib/v8/lib-macosx/libv8_snapshot.a',
            ],
          },
        }],
        ['OS=="win"', {
          'link_settings': {
            'libraries': [
              'v8',
            ],
          },
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalLibraryDirectories': [
                '../lib/v8/lib-msvs',
                '../lib/boost_1_50_msvc_32',
              ],
            },
          },
        }],
      ],
    },
    'sources': [
      '../src/rts/GameScript.cpp',
      '../src/rts/GameScript.h',
      '../src/rts/GameServer.cpp',
      '../src/rts/GameServer.h',
      '../src/rts/Lobby.cpp',
      '../src/rts/Lobby.h',
    ],
  },
  {
    'target_name': 'rts-server',
    'type': 'executable',
    'dependencies': [
      'libcommon',
      'libgame-core',
    ],
    'sources': [
      '../src/exec/server-main.cpp',
    ],
  },
  {
    'target_name': 'rts',
    'type': 'executable',
    'mac_bundle': 1,
    'dependencies': [
      'libcommon',
      'libengine',
      'libgame-core',
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
      '../src/rts/CommandWidget.cpp',
      '../src/rts/CommandWidget.h',
      '../src/rts/EffectFactory.cpp',
      '../src/rts/EffectFactory.h',
      '../src/rts/GameController.cpp',
      '../src/rts/GameController.h',
      '../src/rts/Matchmaker.cpp',
      '../src/rts/Matchmaker.h',
      '../src/rts/MatchmakerController.cpp',
      '../src/rts/MatchmakerController.h',
      '../src/rts/MinimapWidget.cpp',
      '../src/rts/MinimapWidget.h',
      '../src/rts/NativeUIBinding.cpp',
      '../src/rts/NativeUIBinding.h',
      '../src/rts/UIAction.h',

      '../src/rts/Game.cpp',
      '../src/rts/Game.h',
      '../src/rts/GameEntity.cpp',
      '../src/rts/GameEntity.h',
      '../src/rts/Map.cpp',
      '../src/rts/Map.h',
      '../src/rts/Player.cpp',
      '../src/rts/Player.h',
      '../src/rts/PlayerAction.h',
    ],
    'conditions': [
      ['OS=="win"', {
        'include_dirs': [
          '../lib/glew-1.7.0/include',
          '../lib/assimp/include',
        ],
        'link_settings': {
          'libraries': [
            'glew32',
            'opengl32',
            'glfw3dll',
            'assimp',
            'glu32',
          ],
        },
        'msvs_settings': {
          'VCLinkerTool': {
            'OutputFile': '../rts.exe',
            'AdditionalLibraryDirectories': [
              '../lib/assimp/lib-msvc',
              '../lib/glew-1.7.0/lib',
              '../lib/glfw-3.0.1/lib-msvc110',
            ],
          },
        },
        'copies': [{
          'destination': '../',
          'files': [
            '../msvc/DLLs/Assimp32.dll',
            '../msvc/DLLs/glew32.dll',
            '../msvc/DLLs/glfw3.dll',
            '../msvc/DLLs/v8.dll',
          ],
        }],
      }],
      ['OS=="mac"', {
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
          'OTHER_LDFLAGS': [
            '-lGLEW',
            '-lassimp',
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
