const PIECE_SIZE = 4;
const PC_HEIGHT = 4;
const FIELD_HEIGHT = PIECE_SIZE + PC_HEIGHT;
const FIELD_WIDTH = 10;
const PIECE_SHAPES = 7;
const PC_PIECES = PC_HEIGHT * FIELD_WIDTH / PIECE_SIZE;

const MINO_TABLE = [
    [
        [[0, -1], [0, 0], [0, 1], [0, 2]],
        [[-1, 1], [0, 1], [1, 1], [2, 1]],
        [[1, 2], [1, 1], [1, 0], [1, -1]],
        [[2, 0], [1, 0], [0, 0], [-1, 0]]
    ],
    [
        [[-1, -1], [0, -1], [0, 0], [0, 1]],
        [[-1, 1], [-1, 0], [0, 0], [1, 0]],
        [[1, 1], [0, 1], [0, 0], [0, -1]],
        [[1, -1], [1, 0], [0, 0], [-1, 0]]
    ],
    [
        [[0, -1], [0, 0], [0, 1], [-1, 1]],
        [[-1, 0], [0, 0], [1, 0], [1, 1]],
        [[0, 1], [0, 0], [0, -1], [1, -1]],
        [[1, 0], [0, 0], [-1, 0], [-1, -1]]
    ],
    [
        [[0, 0], [-1, 0], [-1, 1], [0, 1]],
        [[0, 0], [-1, 0], [-1, 1], [0, 1]],
        [[0, 0], [-1, 0], [-1, 1], [0, 1]],
        [[0, 0], [-1, 0], [-1, 1], [0, 1]]
    ],
    [
        [[0, -1], [0, 0], [-1, 0], [-1, 1]],
        [[-1, 0], [0, 0], [0, 1], [1, 1]],
        [[0, 1], [0, 0], [1, 0], [1, -1]],
        [[1, 0], [0, 0], [0, -1], [-1, -1]]
    ],
    [
        [[0, -1], [0, 0], [-1, 0], [0, 1]],
        [[-1, 0], [0, 0], [0, 1], [1, 0]],
        [[0, 1], [0, 0], [1, 0], [0, -1]],
        [[1, 0], [0, 0], [0, -1], [-1, 0]]
    ],
    [
        [[-1, -1], [-1, 0], [0, 0], [0, 1]],
        [[-1, 1], [0, 1], [0, 0], [1, 0]],
        [[1, 1], [1, 0], [0, 0], [0, -1]],
        [[1, -1], [0, -1], [0, 0], [-1, 0]]
    ]
];

const COLOR_TABLE = [
    "#00ffff",
    "#3040ff",
    "#ffa500",
    "#ffff00",
    "#00ee00",
    "#ff00ff",
    "#ff0000",
    "#d3d3d3",
    "#000000"
];

const GARBAGE = 7;
const EMPTY = 8;

function Piece(shape, rot, row, col) {
    this.shape = shape;
    this.rot = rot;
    this.row = row;
    this.col = col;

    this.get_mino = function (mino) {
        return [
            row + MINO_TABLE[this.shape][this.rot][mino][0],
            col + MINO_TABLE[this.shape][this.rot][mino][1]
        ];
    };
}

function Field(fhash = 0, fill = GARBAGE) {
    this.grid = Array(FIELD_HEIGHT).fill().map(
        () => Array(FIELD_WIDTH).fill(EMPTY)
    );
    for (let r = FIELD_HEIGHT - 1; r >= 0; r--) {
        for (let c = FIELD_WIDTH - 1; c >= 0; c--) {
            this.grid[r][c] = ((fhash % 2) ? fill : EMPTY);
            fhash = Math.floor(fhash / 2);
        }
    }

    this.lines = 0;

    this.can_place = function (p) {
        for (let mino = 0; mino < PIECE_SIZE; mino++) {
            let pos = p.get_mino(mino);
            if (pos[0] < 0 || pos[0] >= FIELD_HEIGHT || pos[1] < 0 || pos[1] >= FIELD_WIDTH) {
                return false;
            }
            if (this.grid[pos[0]][pos[1]] != EMPTY) {
                return false;
            }
        }
        return true;
    }

    this.place = function (p) {
        for (let mino = 0; mino < PIECE_SIZE; mino++) {
            let pos = p.get_mino(mino);
            this.grid[pos[0]][pos[1]] = p.shape;
        }
    };

    this.clear_lines = function () {
        let num_cleared = 0;
        let flag;
        for (let r = FIELD_HEIGHT - 1; r >= (PIECE_SIZE - num_cleared); r--) {
            if (r >= PIECE_SIZE) {
                for (let c = 0; c < FIELD_WIDTH; c++) {
                    flag = (this.grid[r][c] == EMPTY);
                    if (flag) {
                        break;
                    }
                }
                if (!flag) {
                    num_cleared++;
                }
                else if (num_cleared > 0) {
                    for (let c = 0; c < FIELD_WIDTH; c++) {
                        this.grid[r + num_cleared][c] = this.grid[r][c];
                    }
                }
            }
            else if (r >= 0) {
                for (let c = 0; c < FIELD_WIDTH; c++) {
                    this.grid[r + num_cleared][c] = this.grid[r][c];
                }
            }
            else {
                for (let c = 0; c < FIELD_WIDTH; c++) {
                    this.grid[r + num_cleared][c] = EMPTY;
                }
            }
        }
        for (let r = 0; r < FIELD_HEIGHT; r++) {
            for (let c = 0; c < FIELD_WIDTH; c++) {
                if (this.grid[r][c] != EMPTY) {
                    this.grid[r][c] = GARBAGE;
                }
            }
        }
        this.lines += num_cleared;
    };

    this.disp = function (onclick = "") {
        return `<table class="field" cellspacing="0">` +
            this.grid.slice(PIECE_SIZE).map(
                row => `<tr>` + row.map(
                    sq => `<td style="background:${COLOR_TABLE[sq]}"` +
                        ` onclick="${onclick}"></td>`
                ).join("") + `</tr>`
            ).join("") +
            `</table>`;
    };

    this.hash = function () {
        let h = 0;
        for (let r = PIECE_SIZE + this.lines; r < FIELD_HEIGHT; r++) {
            for (let c = 0; c < FIELD_WIDTH; c++) {
                h *= 2;
                h += (this.grid[r][c] != EMPTY);
            }
        }
        h *= Math.pow(2, FIELD_WIDTH * this.lines);
        h += Math.pow(2, FIELD_WIDTH * this.lines) - 1;
        return h;
    }
}

function interpolate(f1, f2, shape) {
    for (let r = 8; r > 2; r--) {
        for (let c = -1; c < 11; c++) {
            for (let t = 0; t < 4; t++) {
                let p = new Piece(shape, t, r, c);
                if (f1.can_place(p)) {
                    let f3 = new Field(f1.hash());
                    f3.clear_lines();
                    f3.place(p);
                    f3.clear_lines();
                    if (f2.hash() == f3.hash()) {
                        return p;
                    }
                }
            }
        }
    }
}

const PIECE_FIELDS = [
    new Field(535296000, 0),
    new Field(206360015100, 1),
    new Field(12897743100, 2),
    new Field(128974971000, 3),
    new Field(64487670000, 4),
    new Field(51590197500, 5),
    new Field(257949757500, 6)
];

function preview(prev_hash, obj) {
    if (obj[0] === null) {
        return "?";
    }
    if (obj[0] === -1) {
        return (new Field(prev_hash)).disp();
    }
    if (!Array.isArray(obj[0])) {
        let f = new Field(prev_hash);
        f.clear_lines();
        let f2 = new Field(obj[0]);
        f2.clear_lines();
        let p = interpolate(f, f2, obj[1]);
        f.place(p);
        return f.disp();
    }
    let f = new Field(prev_hash);
    f.clear_lines();
    let f2 = new Field(obj[1][0]);
    f2.clear_lines();
    let p = interpolate(f, f2, obj[1][1]);
    f.place(p);
    return f.disp();
}

function disp_options(prev_hash, obj) {
    if (obj == null) {
        document.getElementById("results").innerHTML = "How Did We Get Here?";
        return;
    }
    if (obj[0] === -1) {
        document.getElementById("results").innerHTML = (
            `${(new Field(prev_hash)).disp()}<p>No solution :(</p>`
        );
        return;
    }
    if (!Array.isArray(obj[0])) {
        let f = new Field(prev_hash);
        f.clear_lines();
        let f2 = new Field(obj[0]);
        f2.clear_lines();
        let p = interpolate(f, f2, obj[1]);
        f.place(p);
        let fields = [];
        glob_array = obj[3];
        for (let shape = 0; shape < PIECE_SHAPES; shape++) {
            if (obj[3][shape] != null) {
                fields.push(
                    `<div>` +
                    PIECE_FIELDS[shape].disp(`disp_options(${obj[0]}, glob_array[${shape}])`) +
                    `<br>` +
                    preview(obj[0], obj[3][shape]) +
                    `<p>${Array.isArray(obj[3][shape][0]) ? -obj[3][shape][0][0] : -obj[3][shape][2]}</div>`
                );
            }
            else {
                fields.push(
                    `<div>` +
                    (new Field(PIECE_FIELDS[shape].hash())).disp() +
                    `<p> </p></div>`
                );
            }
        }
        document.getElementById("results").innerHTML = (
            `${f.disp()}<p>${-obj[2]}</p><br>` +
            `<div class="grid">${fields.join("")}</div>`
        );
        return;
    }
    let r = prev_hash;
    let fields = [];
    for (let x of obj.slice(1)) {
        let f1 = new Field(r);
        f1.clear_lines();
        let f2 = new Field(x[0]);
        f2.clear_lines();
        let p = interpolate(f1, f2, x[1]);
        f1.place(p);
        fields.push(f1.disp());
        r = x[0];
    }
    let f = new Field(prev_hash);
    f.clear_lines()
    document.getElementById("results").innerHTML = (
        `${f.disp()}<p>${-obj[0][0]}</p><br>` +
        `<div class="grid">${fields.join("")}</div>`
    );
    return;
}
