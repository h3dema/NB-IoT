# Testing LoRaWAN

You need to create a new NS3 instance to run the lorawan implementation from capacitor-ns3 or lorawan.

## Installation

Consider that you install all the dependencies and programs listed on [install_on_vm.md](install_on_vm.md) and that you have created a base folder to hold the NS3 instances at `~/ns3`.
Then, you need to follow the steps below to run the lorawan implementation on NS-3.

- Download the zip file with NS3 version 3.29 as shown below

```bash
cd ~/ns3
wget https://www.nsnam.org/releases/ns-allinone-3.29.tar.bz2
tar xvf ns-allinone-3.29.tar.bz2
```

- Go to `ns3` folder and prepare (i.e., clean) for the installation:
```
cd ns-allinone-3.29/ns-3.29
./waf clean
```

- Download the LoRaWAN implementation. Below you see the URL to download `capacitor-ns3`. The use the pure lorawan implementation just change the URL. `capacitor-ns3` is a modified implementation of lorawan that make the device batteryless with a harvest source.
```
git clone --depth=1 https://github.com/signetlabdei/capacitor-ns3.git src/lorawan
```


- Configure the build. Notice that we are enabling three extra modules: LTE, LoRaWAN and NetAnim.

```
./waf configure --build-profile=debug --enable-static --disable-examples --enable-modules=lte,netanim,lorawan
./waf build
```

- You can list the modules:
```
./waf list
```

## Examples

If you downloaded `capacitor-ns3`, you can check if it is ok by running (for example) the following code:

```
./waf --run energy-single=device-example
./waf --run network-analysis-capacitor
```
