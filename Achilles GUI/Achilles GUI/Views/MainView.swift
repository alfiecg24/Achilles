//
//  MainView.swift
//  Achilles GUI
//
//  Created by Alfie on 16/08/2023.
//

import SwiftUI

struct MainView: View {
    @State private var index = 0
    @StateObject var settings = AchillesSettings()
    var body: some View {
        ZStack {
            switch index {
            case 0:
                HomeView(index: $index, settings: settings)
            case 1:
                SettingsView(index: $index, settings: settings)
            case 2:
                RunnerView(settings: settings)
            default:
                HomeView(index: $index, settings: settings)
            }
        }
    }
    
}

#Preview {
    MainView()
}
