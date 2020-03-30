# Synthetic Simbotic Plugin
<p align="center"> 
    <img src="docs/labeling/df.gif" width="480">
     <p align="center">Real human face inside a synthetic 3D model</p>
</p>

## Introduction
Synthetic is a plugin that creates synthetic datasets using Simbotic Engine. With this tool is possible to deliver labeled data images or segmented data images with out any effort.

At this moment, Synthetic supports two types of workflows
### Label Images
Generating synthetic data, using PASCAL VOC and YOLO annotation formats.

<p align="center"> 
    <img src="docs/labeling/dc.gif" width="480">
</p>

### Segmentation
Synthetic uses an automatic workflow to generate segmented captures like the following.

<p align="center"> 
    <img src="docs/images/segmentation/capture.png" width="480">
</p>

<p align="center"> 
    <img src="docs/images/segmentation/capture2.png" width="480">
</p>


## Set Up

### Install Simbotic Engine

```
git clone -b Simbotic/4.24 git@github.com:Simbotic/SimboticEngine.git
cd SimboticEngine
./Setup.sh
./GenerateProjectFiles.sh
make
```

## Usage

### Labeling

### Segmentation
To use the segmentation worflow, we need the following:

1. Activate `Enabled with Stencils` inside Simbotic Engine: Project Settings -> Rendering -> Post Processing -> Custom Depth-Stencil Pass.
<p align="center"> 
    <img src="docs/images/segmentation/1.png">
</p>

2. At this moment, the segmentation workflow needs only static meshes actors inside your scene. So, for every actor you need to enable `Render CustomDepth Pass` inside Rendering options.

<p align="center"> 
    <img src="docs/images/segmentation/2.png">
</p>

3. After that, we need to drag our `BP_SegmentationCamera` inside the current scene. 

<p align="center"> 
    <img src="docs/images/segmentation/3.png">
</p>

4. Now, we need to update the following properties as we want. 

<p align="center"> 
    <img src="docs/images/segmentation/4.png">
</p>


5. Inside the segmentation camera blueprint, there is a `SceneCaptureComponent` that we need to modify: In the section PostProcessing Material, we need to select our `PP_SegmentationMaterial` material.

<p align="center"> 
    <img src="docs/images/segmentation/5.png">
</p>

6. Takes capture pressing <kbd>P</kbd> inside your scene. Configure the camera blueprint as you wish.

<p align="center"> 
    <img src="docs/images/segmentation/6.png">
</p>