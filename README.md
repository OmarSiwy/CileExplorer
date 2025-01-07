# Accelerated File Explorer (Cile Explorer)

[](assets/program.png)

Written Completely in C, this project is intended to research the performance that GPU's can provide on mundane applications like file explorers, and see if a possible O(1) Search is possible under the assumption that the number of files will always be less than the GPU cores you can employ.

**Currently Only Supports Linux Systems with NVIDIA GPUs**

# Why Zig instead of Make/CMake?

Zig's build system offers:
- Superior incremental compilation speed compared to Make/CMake
- Clear and flexible build configuration
- Excellent dependency management
- Cross-compilation support out of the box

## Development:

**Non-Linux Users**:
### Prerequisites
- NVIDIA GPU with CUDA support
Good luck building from source! refer to the Dockerfile for any version checking

**Ubuntu**:
```Bash
# To Setup Nvidia-Container Toolkit for Docker
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
  sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
  sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list
sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
sudo docker run --rm --gpus all nvidia/cuda:12.6.0-base-ubuntu20.04 nvidia-smi
```

**Arch**:
```Bash
# To Setup Nvidia-Container Toolkit for Docker
sudo pacman -S nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
sudo docker run --rm --gpus all nvidia/cuda:12.6.0-base-ubuntu20.04 nvidia-smi
```

```Bash
# Using Docker (RECOMMENDED):
xhost +local:docker
docker run --gpus all \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v $HOME/.Xauthority:/root/.Xauthority \
    -v "$(pwd)":/app \
    --network=host \
    -it cileexplorer /bin/bash

# If you have all the dependencies and are brave enough
zig build test --summary all # To Test
zig build run # To Run
zig build cdb # To Build Compile-Commands
```
