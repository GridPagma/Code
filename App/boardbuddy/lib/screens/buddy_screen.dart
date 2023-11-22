import 'dart:async';
import 'dart:math';

import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../widgets/service_tile.dart';
import '../widgets/characteristic_tile.dart';
import '../widgets/descriptor_tile.dart';
import '../utils/snackbar.dart';
import '../utils/extra.dart';

class BuddyScreen extends StatefulWidget {
  final BluetoothDevice device;

  const BuddyScreen({Key? key, required this.device}) : super(key: key);

  @override
  State<BuddyScreen> createState() => _BuddyScreenState();
}

class _BuddyScreenState extends State<BuddyScreen> {
  final String SERVICE_UUID = "ea911a8f-2276-4411-8a1c-be9af3218acb";
  final String CHARACTERISTIC_UUID = "8c3d30e5-476e-4fd8-bec9-5dc5b5e44506";
  BluetoothCharacteristic? target;

  List<int> info = [0, 0, 0, 0];

  int? _rssi;
  int? _mtuSize;
  BluetoothConnectionState _connectionState =
      BluetoothConnectionState.disconnected;
  List<BluetoothService> _services = [];
  bool _isDiscoveringServices = false;
  bool _isConnectingOrDisconnecting = false;

  late StreamSubscription<BluetoothConnectionState>
      _connectionStateSubscription;
  late StreamSubscription<bool> _isConnectingOrDisconnectingSubscription;
  late StreamSubscription<int> _mtuSubscription;

  @override
  void initState() {
    super.initState();

    _connectionStateSubscription =
        widget.device.connectionState.listen((state) async {
      _connectionState = state;
      if (state == BluetoothConnectionState.connected) {
        _services = []; // must rediscover services
      }
      if (state == BluetoothConnectionState.connected && _rssi == null) {
        _rssi = await widget.device.readRssi();
      }
      setState(() {});
    });

    _mtuSubscription = widget.device.mtu.listen((value) {
      _mtuSize = value;
      setState(() {});
    });

    _isConnectingOrDisconnectingSubscription =
        widget.device.isConnectingOrDisconnecting.listen((value) {
      _isConnectingOrDisconnecting = value;
      setState(() {});
    });

    discoverServices();
  }

  @override
  void dispose() {
    _connectionStateSubscription.cancel();
    _mtuSubscription.cancel();
    _isConnectingOrDisconnectingSubscription.cancel();
    super.dispose();
  }

  bool get isConnected {
    return _connectionState == BluetoothConnectionState.connected;
  }

  Future onConnectPressed() async {
    try {
      await widget.device.connectAndUpdateStream();
      discoverServices();
      Snackbar.show(ABC.c, "Connect: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Connect Error:", e),
          success: false);
    }
  }

  Future onDisconnectPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream();
      Snackbar.show(ABC.c, "Disconnect: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Disconnect Error:", e),
          success: false);
    }
  }

  discoverServices() async {
    print("DISCOVERING SERVICES*******************************************");
    setState(() {
      _isDiscoveringServices = true;
    });
    try {
      _services = await widget.device.discoverServices();
      _services.forEach((s) {
        if (s.uuid.toString() == SERVICE_UUID) {
          s.characteristics.forEach((c) {
            if (c.uuid.toString() == CHARACTERISTIC_UUID) {
              target = c;
              print("************TARGET FOUND***************");
            }
          });
        }
      });
      Snackbar.show(ABC.c, "Discover Services: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Discover Services Error:", e),
          success: false);
    }
    setState(() {
      _isDiscoveringServices = false;
    });
  }

  Future onRequestMtuPressed() async {
    try {
      await widget.device.requestMtu(223);
      Snackbar.show(ABC.c, "Request Mtu: Success", success: true);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Change Mtu Error:", e),
          success: false);
    }
  }

  writeData(List<int> data) async {
    if (target == null) return;
    try {
      print("wrote: $data");
      await target?.write(data);
    } catch (e) {
      Snackbar.show(ABC.c, prettyException("Write Error:", e), success: false);
    }
  }

  List<Widget> _buildServiceTiles(BuildContext context, BluetoothDevice d) {
    return _services
        .map(
          (s) => ServiceTile(
            service: s,
            characteristicTiles: s.characteristics
                .map((c) => _buildCharacteristicTile(c))
                .toList(),
          ),
        )
        .toList();
  }

  CharacteristicTile _buildCharacteristicTile(BluetoothCharacteristic c) {
    return CharacteristicTile(
      characteristic: c,
      descriptorTiles:
          c.descriptors.map((d) => DescriptorTile(descriptor: d)).toList(),
    );
  }

  Widget buildSpinner(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(14.0),
      child: AspectRatio(
        aspectRatio: 1.0,
        child: CircularProgressIndicator(
          backgroundColor: Colors.black12,
          color: Colors.black26,
        ),
      ),
    );
  }

  Widget buildConnectOrDisconnectButton(BuildContext context) {
    return TextButton(
        onPressed: isConnected ? onDisconnectPressed : onConnectPressed,
        child: Text(
          isConnected ? "DISCONNECT" : "CONNECT",
          style: Theme.of(context)
              .primaryTextTheme
              .labelLarge
              ?.copyWith(color: Colors.white),
        ));
  }

  Widget buildRemoteId(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Text('${widget.device.remoteId}'),
    );
  }

  Widget buildRssiTile(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        isConnected
            ? const Icon(Icons.bluetooth_connected)
            : const Icon(Icons.bluetooth_disabled),
        Text(((isConnected && _rssi != null) ? '${_rssi!} dBm' : ''),
            style: Theme.of(context).textTheme.bodySmall)
      ],
    );
  }

  Widget buildMtuTile(BuildContext context) {
    return ListTile(
        title: const Text('MTU Size'),
        subtitle: Text('$_mtuSize bytes'),
        trailing: IconButton(
          icon: const Icon(Icons.edit),
          onPressed: onRequestMtuPressed,
        ));
  }

  Widget buildConnectButton(BuildContext context) {
    if (_isConnectingOrDisconnecting) {
      return buildSpinner(context);
    } else {
      return buildConnectOrDisconnectButton(context);
    }
  }

  @override
  Widget build(BuildContext context) {
    // JoystickDirectionCallback onDirectionChanged(
    //     double degrees, double distance) {
    //   print(
    //       "Degree: ${degrees.toStringAsFixed(2)}, distance : ${distance.toStringAsFixed(2)}");
    // }

    return Scaffold(
        appBar: AppBar(
          title: Text('${widget.device.platformName} Test'),
          actions: [buildConnectButton(context)],
        ),
        body: Center(
          child: Joystick(listener: (details) {
            info = [1, 1, 1, 1];
            if (details.x >= 0) {
              info[1] = (details.x * 255).round();
              if (details.y >= 0) {
                info[3] = (details.y * 255).round();
              } else {
                info[2] = -(details.y * 255).round();
              }
            } else {
              info[0] = -(details.x * 255).round();
              if (details.y >= 0) {
                info[3] = (details.y * 255).round();
              } else {
                info[2] = -(details.y * 255).round();
              }
            }
            writeData(info);
          }),
        ));
  }
}
