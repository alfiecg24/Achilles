//
//  HomeView.swift
//  Achilles GUI
//
//  Created by Alfie on 16/08/2023.
//

import SwiftUI

struct HomeView: View {
    @State private var showingSettings = false
    @State private var settingsAreValid = true
    @Binding var index: Int
    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                VStack(alignment: .leading) {
                    Text("Welcome to Achilles")
                        .font(.title)
                    Text("Connect your iOS device to get started")
                        .font(.title3)
                        .foregroundColor(.secondary)
                }
                Spacer()
            }
            .padding()
            Divider()
            HStack {
                VStack(alignment: .leading) {
                    Text("Made by Alfie CG")
                        .font(.title3)
                    Text("With thanks to: the checkra1n team, the palera1n team, 0x7FF and kok3shidoll (among others).")
                    Text("Achilles is open-source on GitHub! Check out the code and build it for yourself [here](https://github.com/alfiecg24/Achilles).")
                        .padding(.top)
                        .font(.title3)
                }
                Spacer()
            }
            .padding()
            Divider()
            HStack {
                VStack(alignment: .leading) {
                    Text("Disclaimer: while it is highly unlikely, I am in no way responsible in the event of damage to a device as a result of using Achilles. By using this program, you acknowledge full responsibility for any issues that may arise.")
                }
                Spacer()
            }
            .padding()
            Divider()
            HStack {
                Spacer()
                Button(action: {
                    withAnimation(.easeInOut) {
                        index = 1
                    }
                }, label: {
                    Text("Options")
                        .frame(width: 75)
                })
                Button(action: {
                    // AchillesStart()
                }, label: {
                    Text("Start")
                        .frame(width: 75)
                })
                .disabled(!($settingsAreValid.wrappedValue))
            }
            Spacer()
        }
        .padding()
        .onAppear {
            if (!checkSettings()) {
                //
            }
        }
//        .sheet(isPresented: $showingSettings, content: {
//            SettingsView()
//                .frame(width: 650, height: 400)
//        })
//        .navigationBarBackButtonHidden()
    }
}

//#Preview {
//    HomeView()
//}

func checkSettings() -> Bool { return true }
