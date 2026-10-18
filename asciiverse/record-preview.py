#!/usr/bin/env python3
"""Records asciiverse/rantaiwarna.cast, the catalog preview, at 100x30
(hub/RECORDING.md): games played by a driver that reads the board from
the screen, finds every group of two or more touching elements of one
colour, and removes the one that scores best now plus the best group it
leaves behind (scores grow with the square of a group's size, so it
favours letting groups grow). It walks the cursor there with the arrow
keys and presses Space. When a game is over it looks at the high score
table (i), starts a new game (n), and quits with q after about 50
seconds.

rantaiwarna runs from out/ (asciiverse/build.sh) by name, so the cast's
header holds no local path, with a throwaway HOME, so nothing touches
your own high scores.

Needs asciinema 3.x and pyte. Usage, from the repo root:
  uv run --with pyte python3 asciiverse/record-preview.py
Any extra argument is used as the cast's path instead (for test runs);
with --dry-run it plays without recording."""
import fcntl, os, pty, select, shutil, struct, sys, tempfile, termios, time
import pyte

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = [a for a in sys.argv[1:] if a != "--dry-run"]
dry = "--dry-run" in sys.argv
cast = args[0] if args else f"{root}/asciiverse/rantaiwarna.cast"
COLS, ROWS, PLAY = 100, 30, 50.0
W, H = 20, 10
UP, DOWN, RIGHT, LEFT = b"\x1bOA", b"\x1bOB", b"\x1bOC", b"\x1bOD"

work = tempfile.mkdtemp(prefix="rantaiwarna-preview-")
env = dict(os.environ, TERM="xterm-256color", COLORTERM="truecolor", LANG="C.UTF-8",
           HOME=work, PATH=f"{root}/out:{os.environ['PATH']}")
for v in ("VTE_VERSION", "TERM_PROGRAM", "TERM_PROGRAM_VERSION", "TMUX", "XDG_DATA_HOME"):
    env.pop(v, None)

pid, fd = pty.fork()
if pid == 0:
    os.chdir(work)
    if dry:
        os.execvpe("rantaiwarna", ["rantaiwarna"], env)
    os.execvpe("asciinema", ["asciinema", "rec", "--overwrite", "-q", "--idle-time-limit", "2",
        "--window-size", f"{COLS}x{ROWS}", "--capture-env", "TERM,COLORTERM,LANG",
        "--title", "rantaiwarna", "--command", "rantaiwarna", cast], env)
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLS, 0, 0))
screen = pyte.Screen(COLS, ROWS)
stream = pyte.ByteStream(screen)
start = time.time()


def pump(seconds):
    end = time.time() + seconds
    while True:
        left = end - time.time()
        r, _, _ = select.select([fd], [], [], max(0.0, min(0.02, left)))
        if r:
            try:
                stream.feed(os.read(fd, 65536))
            except OSError:
                return False
        if left <= 0:
            return True


def key(k, wait=0.08):
    os.write(fd, k)
    pump(wait)


def text():
    return "\n".join(screen.display)


def read_board():
    """{(x, y): element character} inside the board's box, whose top left
    corner is the first box corner on the screen (ncurses draws it from
    the VT100 line set, which pyte keeps as 'l'); each element is two
    columns wide; empty cells are left out."""
    for y, line in enumerate(screen.display):
        for corner in ("lqq", "┌──"):
            x = line.find(corner)
            if x >= 0:
                board = {}
                for r in range(H):
                    for c in range(W):
                        ch = screen.display[y + 1 + r][x + 1 + 2 * c]
                        if ch.strip():
                            board[(c, r)] = ch
                return board
    return None


def groups(board):
    seen, out = set(), []
    for cell in board:
        if cell in seen:
            continue
        group, todo = {cell}, [cell]
        seen.add(cell)
        while todo:
            x, y = todo.pop()
            for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if n in board and n not in seen and board[n] == board[cell]:
                    seen.add(n)
                    group.add(n)
                    todo.append(n)
        if len(group) > 1:
            out.append(group)
    return out


def remove(board, group):
    """The board after removing a group: columns fall, empty columns close."""
    cols = []
    for x in range(W):
        col = [board[(x, y)] for y in range(H) if (x, y) in board and (x, y) not in group]
        if col:
            cols.append(col)
    new = {}
    for x, col in enumerate(cols):
        for i, ch in enumerate(reversed(col)):
            new[(x, H - 1 - i)] = ch
    return new


def best(board):
    choice, value = None, None
    for g in groups(board):
        after = remove(board, g)
        nxt = max((len(h) for h in groups(after)), default=0)
        v = (len(g) - 2) ** 2 + 0.5 * (nxt - 2) ** 2
        if value is None or v > value:
            choice, value = g, v
    return choice


cursor = [0, 0]


def walk_to(cell):
    while cursor[0] < cell[0]:
        key(RIGHT, 0.04)
        cursor[0] += 1
    while cursor[0] > cell[0]:
        key(LEFT, 0.04)
        cursor[0] -= 1
    while cursor[1] < cell[1]:
        key(DOWN, 0.04)
        cursor[1] += 1
    while cursor[1] > cell[1]:
        key(UP, 0.04)
        cursor[1] -= 1


pump(2.0)
games = 0
while time.time() - start < PLAY:
    if "GAME OVER" in text():
        games += 1
        pump(2.0)
        key(b"i", 3.0)     # the high score table
        key(b"a", 0.8)     # back to the game
        key(b"n", 1.2)     # a new game
        cursor = [0, 0]
        continue
    board = read_board()
    g = best(board) if board else None
    if g is None:
        pump(0.3)
        continue
    # the group's cell nearest the cursor
    target = min(g, key=lambda c: abs(c[0] - cursor[0]) + abs(c[1] - cursor[1]))
    walk_to(target)
    pump(0.15)
    key(b" ", 0.45)
score = [l for l in screen.display if "Score" in l]
pump(1.0)
key(b"q", 0.5)
end = time.time() + 5
while time.time() < end:
    if not pump(0.1):
        break
    done, _ = os.waitpid(pid, os.WNOHANG)
    if done:
        break
else:
    os.kill(pid, 9)
try:
    os.waitpid(pid, 0)
except ChildProcessError:
    pass
shutil.rmtree(work, ignore_errors=True)
print(f"{cast}: {time.time() - start:.0f} s, {games} games over, {score[0].split()[-1] if score else '?'}",
      file=sys.stderr)
