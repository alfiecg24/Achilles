//
//  RunnerView.swift
//  Achilles GUI
//
//  Created by Alfie on 17/08/2023.
//

import SwiftUI

struct RunnerView: View {
    @State private var consoleOutput = ""
    @ObservedObject var settings: AchillesSettings
    var body: some View {
        if settings.showLogWindow {
            Text(consoleOutput)
                .multilineTextAlignment(.leading)
                .onAppear {
                    redirectStdoutToTextView()
                }
        }
    }
    private func redirectStdoutToTextView() {
        let pipe = Pipe()
        let fileHandle = pipe.fileHandleForReading
        
        // Redirect stdout to the pipe
        dup2(fileHandle.fileDescriptor, STDOUT_FILENO)
        
        // Read the output from the pipe
        let data = fileHandle.readDataToEndOfFile()
        if let output = String(data: data, encoding: .utf8) {
            consoleOutput = output
        }
    }
}
