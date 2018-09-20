![](https://raw.githubusercontent.com/stackos/Viry3D/master/app/bin/Assets/texture/logo720p.png)

# Viry3D
C++ ��ƽ̨ 3D ��Ϸ���档

֧�� Android��iOS��macOS��Windows��

UWP��Windows ͨ��ƽ̨����

Web������ WebAssembly����

Stack

���䣺stackos@qq.com

QQ ����Ⱥ��428374717

## Build
### Windows
* Visual Studio 2017
* app/project/win/app.sln

### UWP
* Visual Studio 2017
* app/project/uwp/app.sln

### Android
* Android Studio
* app/project/android
* ��ʹ�� Vulkan ������ [https://developer.android.google.cn/ndk/guides/graphics/getting-started.html](https://developer.android.google.cn/ndk/guides/graphics/getting-started.html) ���� shaderc
```
cd (your android sdk dir)\ndk-bundle\sources\third_party\shaderc
..\..\..\ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk \
    APP_STL:=c++_shared APP_ABI=armeabi-v7a APP_PLATFORM=android-18 libshaderc_combined
```
* Python(for copy assets cmd)

### iOS
* Xcode
* app/project/ios/app.xcodeproj

### macOS
* Xcode
* app/project/mac/app.xcodeproj

## ���湦��
�������

    C++11

ƽ̨ & 3D API ֧��

    Vulkan��OpenGL ES 2.0/3.0
    Android��iOS��macOS��Windows��UWP��Windows ͨ��ƽ̨����Web������ WebAssembly��

Mesh

    ʹ�� Unity3D ������������������
    �������ʺ�������

����

    ʹ�� Unity3D ������������
    ֧�ֹ�������
        ��ͬ�������Ȩ�ػ��
        4 ����Ȩ����Ƥ
        ��ƤӲ������
    ���ڱ��������ߵĵ� AnimationCurve

��Ⱦ

    Camera
    Mesh Renderer
    SkinnedMesh Renderer
    Light
    Skybox
    Render To Texture
    FXAA
    PostEffect Blur
    Shadow Map

UI

    Canvas Renderer
    Sprite
    Label
    Freetype Font
    Button

����

    ��ꡢ���̡������¼�����

��Ƶ

    ���� OpenAL �Ŀ�ƽ̨ 3D ��Ƶ����
    ֧�� wav����ʽ mp3 ��ʽ

����

    �ļ� IO
    UTF8��UTF32 �ַ�������
    ��ѧ��

## ���� Demo
[http://www.viry3d.com/](http://www.viry3d.com/)
