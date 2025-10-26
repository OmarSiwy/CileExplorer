{pkgs ? import <nixpkgs> {config.allowUnfree = true;}}:
pkgs.mkShell {
  name = "cuda-environment";
  buildInputs = with pkgs; [
    # Build tools
    wget
    git
    gcc
    gnumake
    zig_0_14

    # GTK3 and dependencies
    gtk3
    glib
    cairo
    pango
    gdk-pixbuf
    harfbuzz
    pkg-config

    # CUDA - specify all the outputs we need
    cudaPackages.cuda_nvcc # Compiler
    cudaPackages.cuda_cudart.dev # Headers (cuda_runtime.h, etc.)
    cudaPackages.cuda_cudart.lib # Runtime libraries
    cudaPackages.cuda_cudart.static # Static libraries (optional)
  ];

  shellHook = ''
    # Create include directory and download stb_image.h
    mkdir -p include
    if [ ! -f include/stb_image.h ]; then
      wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -O include/stb_image.h
    fi

    # Set compiler environment variables
    export CC=${pkgs.gcc}/bin/gcc
    export CXX=${pkgs.gcc}/bin/g++

    # GTK PKG_CONFIG_PATH
    export PKG_CONFIG_PATH="${pkgs.gtk3}/lib/pkgconfig:${pkgs.glib}/lib/pkgconfig:${pkgs.cairo}/lib/pkgconfig:${pkgs.pango}/lib/pkgconfig:${pkgs.gdk-pixbuf}/lib/pkgconfig:${pkgs.harfbuzz}/lib/pkgconfig"

    # CUDA paths - use the .dev output for headers
    export NIX_CUDA_INCLUDE_PATH="${pkgs.cudaPackages.cuda_cudart.dev}/include"
    export NIX_CUDA_LIB_PATH="${pkgs.cudaPackages.cuda_cudart.lib}/lib"

    export CUDA_PATH=${pkgs.cudaPackages.cuda_nvcc}
    export LD_LIBRARY_PATH="$NIX_CUDA_LIB_PATH:$LD_LIBRARY_PATH"
    export PATH="${pkgs.cudaPackages.cuda_nvcc}/bin:$PATH"

    echo "CUDA + Zig development environment loaded"
    echo "CUDA include: $NIX_CUDA_INCLUDE_PATH"
    echo "CUDA lib: $NIX_CUDA_LIB_PATH"
    echo ""
    echo "Checking for cuda_runtime.h:"
    ls -la "$NIX_CUDA_INCLUDE_PATH/cuda_runtime.h" 2>/dev/null && echo "✓ Found!" || echo "✗ Not found"
    echo ""
    echo "nvcc: $(nvcc --version 2>/dev/null | grep release | awk '{print $5}' | sed 's/,//' || echo 'N/A')"
    echo "zig: $(zig version)"
    echo "gcc: $(gcc --version | head -n1)"
  '';
}
