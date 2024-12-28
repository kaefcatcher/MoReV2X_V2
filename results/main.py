# -*- coding: utf-8 -*-
import sys


import pandas as pd
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QComboBox, QSpinBox, QColorDialog, QPushButton, \
    QCheckBox, QHBoxLayout
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
import matplotlib.pyplot as plt


class GraphPlotterApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setStyleSheet("background-color: white;")
        self.setWindowTitle("Graph Plotter App")
        self.setGeometry(100, 100, 500, 350)
        self.graph_settings = {
            "AoI vs PKeep": {"selected": False, "x": "pkeep", "y": "value_aoi", "width": 5, "height": 5, "color": 'blue', "markers": 0},
            "CLR vs PKeep": {"selected": False, "x": "pkeep", "y": "value_clr", "width": 5, "height": 5, "color": 'green', "markers": 0},
            "PAoI vs PKeep": {"selected": False, "x": "pkeep", "y": "value_paoi", "width": 5, "height": 5, "color": 'red', "markers": 0},
            "PDR vs PKeep": {"selected": False, "x": "pkeep", "y": "value_pdr", "width": 5, "height": 5, "color": 'red', "markers": 0},
            "PLR vs PKeep": {"selected": False, "x": "pkeep", "y": "value_plr", "width": 5, "height": 5, "color": 'red', "markers": 0},
            "PDR vs PLR": {"selected": False, "x": "value_plr", "y": "value_pdr", "width": 5, "height": 5, "color": 'red', "markers": 0},
        }

        self.init_ui()

    def init_ui(self):
        # Создаем графический виджет

        # Создаем элементы управления
        self.graph_selectors = {}
        self.checkbox_layout = QVBoxLayout()

        for graph_name, settings in self.graph_settings.items():
            checkbox = QCheckBox(graph_name)
            checkbox.setChecked(settings["selected"])
            checkbox.stateChanged.connect(lambda state, name=graph_name: self.toggle_graph_checkbox(state, name))
            self.graph_selectors[graph_name] = checkbox

            settings_layout = self.create_settings_layout(graph_name)
            self.checkbox_layout.addWidget(checkbox)
            self.checkbox_layout.addLayout(settings_layout)

        plot_button = QPushButton("Построить выбранные графики")
        plot_button.clicked.connect(self.plot_selected_graphs)

        # Создаем компоновку для элементов управления
        control_layout = QVBoxLayout()
        control_layout.addLayout(self.checkbox_layout)
        control_layout.addWidget(plot_button)

        # Создаем основную компоновку
        main_layout = QVBoxLayout()
        main_layout.addLayout(control_layout)


        # Устанавливаем компоновку для окна
        self.setLayout(main_layout)

    def create_settings_layout(self, graph_name):
        settings_layout = QHBoxLayout()

        width_spinbox = QSpinBox()
        width_spinbox.setRange(1, 2000)
        width_spinbox.setValue(self.graph_settings[graph_name]["width"])
        width_spinbox.valueChanged.connect(lambda value, name=graph_name: self.update_graph_settings(name, "width", value))

        height_spinbox = QSpinBox()
        height_spinbox.setRange(1, 2000)
        height_spinbox.setValue(self.graph_settings[graph_name]["height"])
        height_spinbox.valueChanged.connect(lambda value, name=graph_name: self.update_graph_settings(name, "height", value))

        color_button = QPushButton("Выбрать цвет")
        color_button.clicked.connect(lambda name=graph_name: self.choose_color(name))

        markers_spinbox = QSpinBox()
        markers_spinbox.setRange(0, 100)
        markers_spinbox.setValue(self.graph_settings[graph_name]["markers"])
        markers_spinbox.valueChanged.connect(lambda value, name=graph_name: self.update_graph_settings(name, "markers", value))

        settings_layout.addWidget(QLabel("Ширина:"))
        settings_layout.addWidget(width_spinbox)
        settings_layout.addWidget(QLabel("Высота:"))
        settings_layout.addWidget(height_spinbox)

        settings_layout.addWidget(color_button)
        settings_layout.addWidget(QLabel("Маркеры:"))
        settings_layout.addWidget(markers_spinbox)

        return settings_layout

    def choose_color(self, graph_name):
        color = QColorDialog.getColor()
        if color.isValid():
            self.graph_settings[graph_name]["color"] = color.name()

    def toggle_graph_checkbox(self, state, graph_name):
        self.graph_settings[graph_name]["selected"] = state == 2

    def update_graph_settings(self, graph_name, setting_name, value):
        self.graph_settings[graph_name][setting_name] = value

    def plot_selected_graphs(self):
        df = pd.read_csv('Matrix_RC515.csv', delimiter=',')
        # Строим выбранные графики
        for kpi_name, kpi in self.graph_settings.items():
            if kpi["selected"] == True:
                fig = plt.figure(kpi_name, figsize=(kpi["width"],kpi["height"]))

                x = df[kpi["x"]]
                y = df[kpi["y"]]
                plt.plot(x, y, label=kpi_name, color=kpi["color"], marker='o',
                         markersize=kpi["markers"])
                plt.grid()
                plt.legend()
                plt.xlabel(kpi["x"])
                plt.ylabel(kpi["y"])
                plt.show()



if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GraphPlotterApp()
    window.show()
    sys.exit(app.exec_())