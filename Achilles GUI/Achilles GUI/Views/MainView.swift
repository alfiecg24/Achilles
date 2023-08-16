//
//  MainView.swift
//  Achilles GUI
//
//  Created by Alfie on 16/08/2023.
//

import SwiftUI

struct MainView: View {
    @State private var index = 0
    @State var settings = AchillesSettings(verbosity: false, debug: false, quick: false, exploit: false, pongo: false, jailbreak: true, verboseBoot: true, serial: false, bootArguments: "")
    var body: some View {
        ZStack {
            switch index {
            case 0:
                HomeView(index: $index)
            case 1:
                SettingsView(index: $index, settings: $settings)
            default:
                HomeView(index: $index)
            }
        }
    }
    
}

#Preview {
    MainView()
}
