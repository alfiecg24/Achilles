//
//  SettingsView.swift
//  Achilles GUI
//
//  Created by Alfie on 16/08/2023.
//

import SwiftUI

/*
 -v, --verbosity: Verbosity level, maximum of 2 (e.g. -vv, --verbosity 2)
 -d, --debug: Enable debug logging
 -h, --help: Show this help message
 -V, --version: Show version information
 -q, --quick: Don't ask for confirmation during the program
 -e, --exploit: Exploit with checkm8 and exit
 -p, --pongo: Boot to PongoOS and exit
 -j, --jailbreak: Jailbreak rootless using palera1n kpf, ramdisk and overlay
 -V, --verbose-boot: Boot device with verbose boot
 -s, --serial: Enable serial output from the device when booting
 -b, --boot-args: Boot arguments to pass to PongoOS
 -k, --override-pongo: Use a custom Pongo.bin file
*/

struct AchillesSettings {
    var verbosity: Bool
    var debug: Bool
    var quick: Bool
    var exploit: Bool
    var pongo: Bool
    var jailbreak: Bool
    var verboseBoot: Bool
    var serial: Bool
    var bootArguments: String
}

struct SettingsView: View {
    @Binding var index: Int
    @Binding var settings: AchillesSettings
    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                VStack(alignment: .leading) {
                    Text("\(NAME) v\(VERSION), \(RELEASE_TYPE)")
                        .font(.title3)
                        .foregroundColor(.secondary)
                    Section(content: {
                        Toggle(isOn: $settings.verbosity, label: {
                            Text("Verbose logging")
                        })
                        Toggle(isOn: $settings.debug, label: {
                            Text("Debug logging")
                        })
                        Toggle(isOn: $settings.quick, label: {
                            Text("Quick mode")
                        })
                    }, header: {
                        Text("General")
                            .foregroundColor(.secondary)
                            .padding(.top, 10)
                    })
                    Section(content: {
                        Toggle(isOn: $settings.exploit, label: {
                            Text("Exploit only (pwned DFU mode)")
                        })
                        .disabled($settings.pongo.wrappedValue || $settings.jailbreak.wrappedValue)
                        Toggle(isOn: $settings.pongo, label: {
                            Text("Boot to PongoOS and exit")
                        })
                        .disabled($settings.exploit.wrappedValue || $settings.jailbreak.wrappedValue)
                        Toggle(isOn: $settings.jailbreak, label: {
                            Text("Jailbreak device (iOS 15+)")
                        })
                        .disabled($settings.exploit.wrappedValue || $settings.pongo.wrappedValue)
                    }, header: {
                        Text("Exploit type")
                            .foregroundColor(.secondary)
                        .padding(.top, 10)
                    })
                    Section(content: {
                        Toggle(isOn: $settings.verboseBoot, label: {
                            Text("Verbose boot")
                        })
                        .disabled((!($settings.pongo.wrappedValue) && !($settings.jailbreak.wrappedValue)) || $settings.serial.wrappedValue)
                        Toggle(isOn: $settings.serial, label: {
                            Text("Serial output")
                        })
                        .disabled((!($settings.pongo.wrappedValue) && !($settings.jailbreak.wrappedValue)) || $settings.verboseBoot.wrappedValue)
                        TextField("Additional boot-args", text: $settings.bootArguments)
                            .disabled(!($settings.pongo.wrappedValue) && !($settings.jailbreak.wrappedValue))
                    }, header: {
                        Text("PongoOS boot options")
                            .foregroundColor(.secondary)
                        
                            .padding(.top, 10)
                    })
                }
                Spacer()
            }
            .padding()
            Divider()
            HStack {
                Spacer()
                Button("Dismiss") {
                    withAnimation(.easeInOut) {
                        index = 0
                    }
                }
            }
            .padding()
        }
    }
}

//#Preview {
//    SettingsView()
//}
