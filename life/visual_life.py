#!/usr/bin/python3
#
#   Visualiser for Conway's Game of Life
#   Copyright (C) 2025 Nik Sultana

#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# $ python3 visual_life.py

import argparse
import curses
import time


parser = argparse.ArgumentParser(
                    prog='LifeViz',
                    description="Visualiser for Conway's Game of Life",
                    epilog="This program is part of the materials used in this course: http://www.cs.iit.edu/~nsultana1/teaching/S26CS451/ Please send any feedback to the course instructor, Nik Sultana.")

parser.add_argument('filename')
parser.add_argument('--initial_pause', type=int, default=0)
parser.add_argument('--pause', type=float, default=0.5)
parser.add_argument('--win_start_x', type=int, default=0)
parser.add_argument('--win_start_y', type=int, default=0)
parser.add_argument('--win_end_x', type=int, default=0)
parser.add_argument('--win_end_y', type=int, default=0)

args = parser.parse_args()

assert(args.win_start_y <= args.win_end_y)
assert(args.win_start_x <= args.win_end_x)


def process_step(step_data, stdscr):
    y = 1
    for line in step_data:

      if args.win_start_y != 0 or args.win_end_y != 0:
        if y - 1 < args.win_start_y: continue
        if y - 1 > args.win_end_y: continue

      x = 0
      for cell in line:

        if args.win_start_x != 0 or args.win_end_x != 0:
          if x < args.win_start_x: continue
          if x > args.win_end_x: continue

        if 1 == cell:
          stdscr.addstr(y, x, u"\u2592", curses.color_pair(2) | curses.A_BOLD)
        elif 0 == cell:
          stdscr.addstr(y, x, u"\u2592", curses.color_pair(1))
        else:
          raise Exception('Invalid cell value')
        x = x + 1
      y = y + 1


def main(stdscr):
  curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)
  curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
  curses.init_pair(3, curses.COLOR_CYAN, curses.COLOR_BLACK)
  curses.init_pair(4, curses.COLOR_RED, curses.COLOR_BLACK)

  curses.curs_set(0)

  stdscr.clear()
  stdscr.addstr(0, 0, "LifeViz", curses.color_pair(3))

  step_number = 0
  with open(args.filename) as file:
    for line in file:
      row = list(map((lambda l: list(map(lambda x: int(x), l.split(',')))), line.split()))
      process_step(row, stdscr)
      stdscr.addstr(0, len("LifeViz") + 1, "step " + str(step_number), curses.color_pair(2) | curses.A_BOLD)
      stdscr.refresh()
      time.sleep(args.pause)
      if 0 != args.initial_pause and 0 == step_number:
        time.sleep(args.initial_pause)
      step_number = step_number + 1

  stdscr.addstr(0, len("LifeViz") + 1 + len("step") + 1 + len(str(step_number)) + 1, "(Done)", curses.color_pair(4) | curses.A_BOLD)

  stdscr.getch()


curses.wrapper(main)
