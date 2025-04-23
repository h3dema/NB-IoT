
# Set-up the whole environment

## Create VM Ubuntu in Hyper-V


To create an **Ubuntu virtual machine** in **Hyper-V**, follow these steps.
This procedure assumes you're running Hyper-V on Windows 10/11.


### Prerequisites

- **Hyper-V enabled** on your machine. If not:
  - Open PowerShell as Administrator and run:
    ```powershell
    dism.exe /Online /Enable-Feature /All /FeatureName:Microsoft-Hyper-V
    ```
  - Reboot after enabling.

- **Ubuntu ISO file**: Download from [ubuntu.com/download/desktop](https://ubuntu.com/download/desktop)

> You can use "Quick create" to install Ubuntu but here I'm showing a process that gives you a bit more control over the creation.


### Steps to Create the Ubuntu VM

#### 1. Open Hyper-V Manager

- Press `Windows + S`, search for **Hyper-V Manager**, and open it.

#### 2. Create a New Virtual Machine

- In **Hyper-V Manager**, click **New > Virtual Machine** (right pane).
- Follow the wizard:
  - **Name your VM** (e.g., `NS-3`).
  - Choose to **store it in a specific location** (optional). **I prefer to install it in my user's folder**.
  - **Generation**: Select **Generation 2** (for UEFI and Secure Boot; Ubuntu supports it).
  - **Startup Memory**: Set at least **4096 MB (4 GB)** or more.
  - **Networking**: Choose an existing virtual switch (or set one up later).
  - **Virtual Hard Disk**: Create a new one (e.g., 20GB or more).
  - **Installation Options**: Select **Install an operating system from a bootable image file**, then **browse to the Ubuntu ISO**.

#### 3. Adjust VM Settings

Before starting the VM, tweak:
- **Processor**: Increase to 2 or more virtual processors.
- **Secure Boot**: If using **Generation 2**, go to **Security** settings and **uncheck "Enable Secure Boot"** or change the template to "Microsoft UEFI Certificate Authority" to allow Ubuntu to boot. <span style="color: red">I'm using the secure boot, so it is mandatory to change the template.</span> Otherwise, Ubuntu does not boot.
- **Automatic Checkpoints**: You can disable if you don’t want auto-snapshots. <span style="color: red">I have this disabled</span>.

#### 4. Start the VM

- Right-click the VM → **Connect** → **Start**.
- Ubuntu installer will launch. Proceed with Ubuntu installation as normal.

#### 5. Install Hyper-V Integration Services (Enhanced Session)

Ubuntu supports **Enhanced Session** via **xRDP**, but you may need to:
- Install guest services:
  ```bash
  sudo apt update
  sudo apt install -y xrdp
  ```
- Enable and start xRDP:
  ```bash
  sudo systemctl enable xrdp
  sudo systemctl start xrdp
  ```



## Install packages to run `ns-3`



```bash
apt -y update && apt install -y \
    g++ \
    python3 \
    cmake \
    ninja-build \
    git \
    vim \
    ccache \
    clang-format clang-tidy \
    gdb valgrind
```


```bash
DEBIAN_FRONTEND=noninteractive apt install -y \
    tcpdump wireshark \
    sqlite sqlite3 libsqlite3-dev \
    qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    doxygen graphviz imagemagick \
    python3-sphinx dia imagemagick texlive dvipng latexmk texlive-extra-utils texlive-latex-extra texlive-font-utils
```


```bash
apt install -y gsl-bin libgsl-dev libgslcblas0 \
    libxml2 libxml2-dev \
    libgtk-3-dev \
    python3-pip python3-dev python3-setuptools
```

```bash
python3 -m pip install --user cppyy==3.1.2
```

```bash
apt install -y gir1.2-goocanvas-2.0 python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython3 wget
```

#### Create folder to hold ns-3 and netanim


```bash
mkdir -p ~/ns3
```


## Install netanim

#### Install dependencies

If you read the instruction in [Debian/Ubuntu Linux distribution](https://www.nsnam.org/wiki/NetAnim_3.108#Debian/Ubuntu_Linux_distribution:), you will see that the page instructs to install two packages: mercurial and qt5-default. However, qt5-default does not exist in Ubuntu 22.04.

Install the following packages:

```bash
sudo apt -y install mercurial qtcreator qtbase5-dev qt5-qmake cmake
```

#### Clone the repository

```bash
cd ~/ns3
hg clone http://code.nsnam.org/netanim
```

> More info at [Downloading NetAnim](https://www.nsnam.org/wiki/NetAnim_3.108#Downloading_NetAnim)


#### Make the code

To create the program, you have to make it:

```bash
cd ~/ns3/netanim
make clean
qmake NetAnim.pro
make
```

> PS: it is normal to see some warnings during `make`.

Let's add it to the path so 

```bash
sudo ln -s `pwd`/NetAnim /usr/bin
```

## Install ns3

We are going to install a modified ns-3.
It was changed by IDLab to implement eDRX on NB-IoT.


#### Clone NB-IoT

There are three branches in this repository.
We are going to focus on the `energy_evaluation`:


```bash
cd ~/ns3/
git clone https://github.com/h3dema/NB-IoT.git
cd NB-IoT
git checkout energy_evaluation
```

> PS: [Original repository](https://github.com/imec-idlab/NB-IoT.git)


#### Add lorawan

This repository cannot run a newer lorawan implementation from [lorawan](https://github.com/signetlabdei/lorawan) or [capacitor-ns3](https://github.com/signetlabdei/capacitor-ns3/).
Check [test_lorawan.md](test_lorawan.md) to see how you can install a separate NS3 instance to run LoRaWAN.

If you can test with an older version of lorawan
```

git clone --depth=1 https://github.com/signetlabdei/lorawan.git src/lorawan
cd src/lorawan
git checkout 0.1.0
```

#### compile the code

- configure

```bash
cd ~/ns3/NB-IoT
CXXFLAGS="-std=c++0x -Wall -g -O0" ./waf configure --build-profile=debug --enable-static --enable-examples --enable-modules=lte,netanim
```

> If you decided to download LoRaWAN, you must add `lorawan` to `--enable-modules` after `netanim`.

- make

```bash
./waf 
```

#### test the installation

Run the simulation: 
```bash
./waf --run lena-simple-epc-1
```



## Give access to VM using WSL2

If you want to access your Hyper-V VM from WSL (for example, a local Ubuntu on WSL2), you cannot by default because the IP from Hyper-V are not forwarded to WSL2 shells.
The process is quite simple:

1. Open Windows PowerShell as administrator.

2. In the command prompt, run the following command:

```
Get-NetIPInterface | where {$_.InterfaceAlias -eq 'vEthernet (WSL (Hyper-v firewall))' -or $_.InterfaceAlias -eq 'vEthernet (Default Switch)'} | Set-NetIPInterface -Forwarding Enabled
```

You may need to change 'vEthernet (WSL (Hyper-v firewall))' depending on what is your local network configuration.
In the commando prompt, run `ipconfig`. It lists several interfaces.
Mine looks like below. Check the name after `Ethernet adapter`. This is the entry you must write in the powershell command above.

<pre>
Ethernet adapter vEthernet (WSL (Hyper-V firewall)):

   Connection-specific DNS Suffix  . :
   Link-local IPv6 Address . . . . . : fe80::5935:730c:9b6d:2966%75
   IPv4 Address. . . . . . . . . . . : 172.28.96.1
   Subnet Mask . . . . . . . . . . . : 255.255.240.0
   Default Gateway . . . . . . . . . :
</pre>

