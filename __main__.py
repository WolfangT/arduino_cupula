# archivo: __main__.py

from time import sleep
from threading import Lock
from os import path
import sys

from flask import Flask, render_template, request, redirect

from .cupula import Cupula

control = Cupula()
app = Flask(__name__)
lock = Lock()


@app.route('/')
def main():
    return render_template('main.html', **control.datos)


@app.route('/girar', methods=['POST'])
def girar():
    print('girando')
    try:
        grados = int(request.form['grados'])
    except ValueError:
        grados = 0
    with lock:
        control.girar(grados)
    return redirect('/')


@app.route('/ventana', methods=['POST'])
def ventana():
    print('cambiando estado ventana, {}'.format(control.ventana))
    with lock:
        if control._ventana == 1:
            control.cerrar()
        elif control._ventana == -1:
            control.abrir()
        else:
            print('no se puede cambiar ventana si esta en espera')
    sleep(0.5)
    return redirect('/')


@app.route('/modo_automatico', methods=['POST'])
def modo_automatico():
    print(
        'cambiando modo automatico, estado actual es <{}>'.format(
            control.modo_automatico))
    with lock:
        if control._modo_automatico == 1:
            control.cambiar_modo_seguimiento()
        else:
            control.cambiar_modo_remoto()
    sleep(0.5)
    return redirect('/')


def run_interface():
    try:
        app.run(host='0.0.0.0')
    except:
        cupula.kill()
        raise
