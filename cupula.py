# archivo: cupula.py

import sys
from serial import Serial
from threading import Thread, Lock
from time import sleep, time
import random

#TODO Segun OS
PUERTO = '\\.\COM5'

#PUERTO = '/dev/tty'


class Cupula(Thread):
    def __init__(self):
        Thread.__init__(self, name="ServidorCupula", daemon=True)
        self.serial = Serial(PUERTO, 9600)
        self.ejecutarse = True
        self.enviando = Lock()
        self._datos = {
            'cupula': None,
            'telescopio': None,
            'ventana': None,
            'direccion': None,
            'stop': None,
            'modo': None,
            'modo_automatico': None,
            'temperatura': None,
            'humedad': None,
            'viento': None,
        }
        self.start()

    @property
    def datos(self):
        return {
            'cupula': self.cupula,
            'telescopio': self.telescopio,
            '_ventana': self._ventana,
            'ventana': self.ventana,
            'direccion': self.direccion,
            '_stop': self._stop,
            'stop': self.stop,
            'modo': self.modo,
            'modo_automatico': self.modo_automatico,
            'temperatura': self.temperatura,
            'humedad': self.humedad,
            'viento': self.viento,
        }

    def run(self):
        while self.ejecutarse:
            try:
                linea = self.serial.readline()
                datos = linea.decode('ascii')
            except UnicodeDecodeError:
                print(
                    'Error en lectura de Datos desde el Maestro: {!r}'.format(
                        linea))
                continue
            if datos:
                var, val = datos.split(':', 1)
                if var == "debug":
                    print(val)
                else:
                    self._datos[var] = val

    def enviarDatos(self, *datos):
        datos = [str(dato) for dato in datos]
        info = ':'.join(datos)
        envio = bytes(info, 'ascii')
        with self.enviando:
            self.serial.write(envio)

    def terminar(self):
        self.ejecutarse = False

    # Datos del arduino

    @property
    def cupula(self):
        return '{}°'.format(self._datos['cupula'])

    @property
    def telescopio(self):
        return '{}º'.format(self._datos['telescopio'])

    @property
    def viento(self):
        #self._datos['viento'] = 3 + (random.randint(-10, 10)/4)
        return '{} m/s'.format(self._datos['viento'])

    @property
    def temperatura(self):
        #self._datos['temperatura'] = 24 + (random.randint(-10, 10)/4)
        return '{} ºC'.format(self._datos['temperatura'])

    @property
    def humedad(self):
        #self._datos['humedad'] = 65.5 + (random.randint(-10, 10)/4)
        return '{}%'.format(self._datos['humedad'])

    @property
    def _ventana(self):
        return int(self._datos['ventana'])

    @property
    def ventana(self):
        estado = {
            1: 'Abierta',
            0: 'Espera',
            -1: 'Cerrada',
        }
        return 'La ventana esta {}'.format(estado[self._ventana])

    @property
    def _direccion(self):
        return int(self._datos['direccion'])

    @property
    def direccion(self):
        estado = {
            1: 'girando en sentido Anti-Horario',
            0: 'Parada',
            -1: 'girando en sentido Horario',
        }
        return 'La cupula esta {}'.format(estado[self._direccion])

    @property
    def _stop(self):
        return int(self._datos['stop'])

    @property
    def stop(self):
        estado = 'Activada' if self._stop else 'Desactivada'
        return 'La Parada de Emergencia esta {}'.format(estado)

    @property
    def _modo_automatico(self):
        return int(self._datos['modo_automatico'])

    @property
    def modo_automatico(self):
        estado = 'Remoto' if self._modo_automatico else 'Seguimiento'
        return ' {}'.format(estado)

    @property
    def _modo(self):
        return int(self._datos['modo'])

    @property
    def modo(self):
        estado = 'Automatico' if self._modo else 'Manual'
        return 'Está en modo {}'.format(estado)

    # Metodos control

    def girar(self, grados):
        print('ejecutando comando girar')
        self.enviarDatos('cupula', float(grados))

    def abrir(self):
        print('ejecutando comando abrir')
        self.enviarDatos('ventana', 'abrir')

    def cerrar(self):
        print('ejecutando comando cerrar')
        self.enviarDatos('ventana', 'cerrar')

    def cambiar_modo_seguimiento(self):
        print('ejecutando comando cambio de modo a Seguimiento')
        self.enviarDatos('modo_automatico', 'seguimiento')

    def cambiar_modo_remoto(self):
        print('ejecutando comando cambio de modo a Remoto')
        self.enviarDatos('modo_automatico', 'remoto')


if __name__ == '__main__':
    cupula = Cupula()
    try:
        pass
    except Exception as err:
        cupula.servidor.terminar()
        raise err
