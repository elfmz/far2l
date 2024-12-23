__all__ = ["Window", "Spacer", "HSizer", "VSizer", "GridSizer"]


class Orientation:
    horizontal = 0
    vertical = 1


class Window:
    def __init__(self, pos=(0, 0), size=(0, 0)):
        self.pos = pos
        self.size = size

    def move(self, l, t, w, h):
        self.pos = (l, t)
        self.size = (w, h)

    def get_best_size(self):
        raise NotImplementedError("Window.get_best_size")


class Spacer(Window):
    def get_best_size(self):
        return self.size

class Box:
    def __init__(self, window, border):
        self.window = window
        self.border = border
        assert len(border) == 4

    def size(self, l, t, r, b):
        w = r - l
        h = b - t
        self.window.move(l, t, w, h)

    def get_best_size(self):
        x, y = self.window.get_best_size()
        return (
            x + self.border[0] + self.border[2],
            y + self.border[1] + self.border[3],
        )


class Sizer:
    orientation = None

    def __init__(self, border=(0, 0, 0, 0)):
        assert self.orientation in [Orientation.horizontal, Orientation.vertical]
        self.boxes = []
        self.border = border

    def add(self, w, border=(0, 0, 0, 0)):
        self.boxes.append(Box(w, border))

    def move(self, x, y, w, h):
        self.size(x, y, w, h)

    def size(self, l, t, r, b):
        l += self.border[0]
        t += self.border[1]
        r -= self.border[2]
        b -= self.border[3]
        hoffset = l
        voffset = t
        for box in self.boxes:
            cx, cy = box.get_best_size()
            if self.orientation == Orientation.horizontal:
                box.size(hoffset, voffset, hoffset + cx, voffset + cy)
                hoffset += cx
            elif self.orientation == Orientation.vertical:
                box.size(hoffset, voffset, hoffset + cx, voffset + cy)
                voffset += cy

    def get_best_size(self):
        b_x = 0
        b_y = 0
        for box in self.boxes:
            cx, cy = box.get_best_size()
            if self.orientation == Orientation.horizontal:
                b_x += cx
                b_y = max(cy, b_y)
            elif self.orientation == Orientation.vertical:
                b_x = max(cx, b_x)
                b_y += cy
        return b_x, b_y


class HSizer(Sizer):
    orientation = Orientation.horizontal


class VSizer(Sizer):
    orientation = Orientation.vertical


class FlowSizer(Sizer):
    def __init__(self, cols, border=(0, 0, 0, 0)):
        self.boxes = [[]]
        self.cols = cols
        self.border = border
        self.sizes = None
        self.col_widths = None
        self.row_heights = None

    def add(self, w, border=(0, 0, 1, 0)):
        box = self.boxes[-1]
        if len(box) == self.cols:
            box = []
            self.boxes.append(box)
        box.append(Box(w, border))

    def move(self, x, y, w, h):
        self.size(x, y, w, h)

    def size(self, l, t, r, b):
        l += self.border[0]
        t += self.border[1]
        r -= self.border[2]
        b -= self.border[3]
        cy = 0
        for row in range(len(self.boxes)):
            cx = 0
            for col in range(len(self.boxes[row])):
                box = self.boxes[row][col]
                bx, by = box.get_best_size()
                box.size(l+cx, t+cy, l+cx+self.col_widths[col], t+cy+by)
                cx += self.col_widths[col]
            cy += self.row_heights[row]

    def get_best_size(self):
        while len(self.boxes[-1])%self.cols:
            self.add(Spacer())
        sizes = [[col.get_best_size() for col in row] for row in self.boxes]
        col_widths = [max(sizes[row_nr][col_nr][0] for row_nr in range(len(sizes))) for col_nr in range(len(sizes[0]))]
        row_heights = [max(col[1] for col in row) for row in sizes]
        self.sizes = sizes
        self.col_widths = col_widths
        self.row_heights = row_heights
        w = sum(col_widths)
        h = sum(row_heights)
        return w, h
