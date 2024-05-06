import subprocess
import sys
import platform

def install_dependencies():
    os_name = platform.system()
    try:
        if os_name == "Darwin":  # macOS
            print("Installing dependencies on macOS using Homebrew...")
            # Check if Homebrew is installed, install if it's not
            brew_installed = subprocess.call(["which", "brew"]) == 0
            if not brew_installed:
                subprocess.check_call('/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"', shell=True)
            subprocess.check_call(["brew", "install", "pkg-config", "gtk+3"])

        elif os_name == "Linux":
            print("Installing dependencies on Linux...")
            # Update system and install GTK+3
            subprocess.check_call(["sudo", "apt-get", "update"])
            subprocess.check_call(["sudo", "apt-get", "install", "-y", "pkg-config", "libgtk-3-dev"])
            # Install CUDA (use specific version as needed, this example installs the latest)
            print("Installing CUDA...")
            subprocess.check_call(["sudo", "apt-get", "install", "-y", "nvidia-cuda-toolkit"])
            print("CUDA installed successfully.")

        elif os_name == "Windows":
            print("Note: Ensure MSYS2 is installed and set up pkg-config manually in Windows.")
            print("For CUDA installation, please download the installer from the NVIDIA website and follow the setup instructions.")

        else:
            print(f"Unsupported operating system: {os_name}")
            sys.exit(1)

        print("Dependencies installed successfully.")
    except subprocess.CalledProcessError as e:
        print(f"An error occurred while installing dependencies: {e}")
        sys.exit(1)

if __name__ == "__main__":
    install_dependencies()
