//
//  View++.swift
//  Achilles GUI
//
//  Created by Alfie on 16/08/2023.
//

import SwiftUI

extension View {
    func popup<T: View>(isPresented: Bool, @ViewBuilder content: () -> T) -> some View {
        modifier(Popup(isPresented: isPresented, content: content))
    }
}

struct Popup<T: View>: ViewModifier {
    let popup: T
    let isPresented: Bool

    init(isPresented: Bool, @ViewBuilder content: () -> T) {
        self.isPresented = isPresented
        popup = content()
    }

    func body(content: Content) -> some View {
        content
            .overlay(popupContent())
    }

    @ViewBuilder private func popupContent() -> some View {
        GeometryReader { geometry in
            if isPresented {
                popup
                    .frame(width: geometry.size.width, height: geometry.size.height)
            }
        }
    }
}
