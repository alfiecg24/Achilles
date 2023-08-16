//
//  Scene++.swift
//  Achilles GUI
//
//  Created by Alfie on 16/08/2023.
//

import SwiftUI

extension Scene {
    func windowResizabilityContentSize() -> some Scene {
        if #available(macOS 13.0, *) {
            return windowResizability(.contentSize)
        } else {
            return self
        }
    }
}
