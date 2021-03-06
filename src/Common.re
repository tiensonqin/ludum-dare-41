let debug = false;

type plantT = int;

type tileT =
  | Dirt
  | Grass(int)
  | Fence(char)
  | Floor
  | Water
  | Blocked
  | SeedBin
  | WaterTrough
  | FoodTrough
  | Hay
  | Truck;

type directionT =
  | UpD
  | DownD
  | RightD
  | LeftD;

type vec2 = {
  x: float,
  y: float,
};

type carryableT =
  | Seed
  | Water
  | Milk
  | Egg
  | Corn
  | Wood
  | Axe
  | Flower;

type actionT =
  | PickUp(carryableT)
  | Cleanup
  | CleanupBlood
  | WaterCorn
  | WaterAnimals
  | FeedAnimals
  | PlantSeed
  | PutBackWater
  | PutBackSeed
  | Sell
  | DoBarnDoor
  | GoToBed
  | InspectTombstone
  | NoAction;

type tankStateT =
  | HalfFull
  | Full
  | Empty;

type cowStateT = {
  momentum: vec2,
  health: int,
};

type chickenStateT = {
  momentum: vec2,
  health: int,
  willDie: bool,
};

type barnDoorT =
  | Broken
  | Opened
  | Closed;

type bossStateT = {
  movePair: (vec2, vec2),
  movingTime: float,
  hunger: int,
  eatingTime: float,
  killed: list(gameobjectT),
  eating: bool,
}
and gameobjectStateT =
  | Corn(int)
  | Cow(cowStateT)
  | WaterTank(tankStateT)
  | FoodTank(tankStateT)
  | Chicken(chickenStateT)
  | Boss(bossStateT)
  | Chick(chickenStateT)
  | IsASeedBin
  | HayBale
  | NoState
  | BarnDoor(barnDoorT)
  | Tombstone(bool)
  | AxeStanding
and gameobjectT = {
  pos: vec2,
  action: actionT,
  state: gameobjectStateT,
};

let posMake = (x, y) => {x: float_of_int(x), y: float_of_int(y)};

module StringMap = Map.Make(String);

type assetT = {
  size: vec2,
  pos: vec2,
};

type journalEntryT = string;

type dayTransitionT =
  | NoTransition
  | CheckJournal
  | JournalIn
  | JournalOut
  | FadeOut
  | FadeIn;

type journalT = {
  dayIndex: int,
  dayTransition: dayTransitionT,
  animationTime: float,
  pageNumber: int,
};

type stateT = {
  grid: array(array(tileT)),
  plants: array(array(plantT)),
  playerPos: vec2,
  playerFacing: directionT,
  spritesheet: Reprocessing.imageT,
  assets: StringMap.t(assetT),
  sounds: StringMap.t((Reprocessing.soundT, float)),
  currentItem: option(carryableT),
  gameobjects: list(gameobjectT),
  journal: journalT,
  dollarAnimation: float,
  time: float,
  night: bool,
  mainFont: Reprocessing.fontT,
  monsterWasLockedIn: bool,
  mousePressed: bool,
  mousePressedHack: bool,
  day6PlayerWentInBarn: bool,
  day6CameraAnimation: float,
  shortDestroyedBarnAsset: Reprocessing.imageT,
  sleepingMonsterAsset: Reprocessing.imageT,
  hasPressedTheActionKeyOnce: bool,
  playerDead: bool,
};

let screenSize = 600.;

let playerSpeed = 150.;

let tileSize = 32;

let tileSizef = float_of_int(tileSize);

let drawAssetFullscreen = (name, state, env) =>
  switch (StringMap.find(name, state.assets)) {
  | exception Not_found =>
    print_endline(
      "Asset " ++ name ++ " not found. Get your shit together man.",
    )
  | asset =>
    Reprocessing.Draw.subImage(
      state.spritesheet,
      ~pos=(0, 0),
      ~width=Reprocessing.Env.width(env),
      ~height=Reprocessing.Env.height(env),
      ~texPos=(int_of_float(asset.pos.x), int_of_float(asset.pos.y)),
      ~texWidth=int_of_float(asset.size.x),
      ~texHeight=int_of_float(asset.size.y),
      env,
    )
  };

let drawAsset = (x, y, name, state, env) =>
  switch (StringMap.find(name, state.assets)) {
  | exception Not_found =>
    print_endline(
      "Asset " ++ name ++ " not found. Get your shit together man.",
    )
  | asset =>
    Reprocessing.Draw.subImage(
      state.spritesheet,
      ~pos=(x, y),
      ~width=int_of_float(asset.size.x),
      ~height=int_of_float(asset.size.y),
      ~texPos=(int_of_float(asset.pos.x), int_of_float(asset.pos.y)),
      ~texWidth=int_of_float(asset.size.x),
      ~texHeight=int_of_float(asset.size.y),
      env,
    )
  };

let drawAssetf = (x, y, name, state, env) =>
  switch (StringMap.find(name, state.assets)) {
  | exception Not_found =>
    print_endline(
      "Asset " ++ name ++ " not found. Get your shit together man.",
    )
  | asset =>
    Reprocessing.Draw.subImagef(
      state.spritesheet,
      ~pos=(x, y),
      ~width=asset.size.x,
      ~height=asset.size.y,
      ~texPos=(int_of_float(asset.pos.x), int_of_float(asset.pos.y)),
      ~texWidth=int_of_float(asset.size.x),
      ~texHeight=int_of_float(asset.size.y),
      env,
    )
  };

let soundNames = [
  ("day1", 1.0),
  ("day2", 1.0),
  ("day3", 1.0),
  ("day4", 1.0),
  ("day5", 1.0),
  ("night1", 1.0),
  ("night2", 1.0),
  ("night3", 1.0),
  ("night4", 1.0),
  ("night5", 1.0),
  ("monster1", 1.0),
  ("monster2", 1.0),
  ("door_scratch1", 1.0),
  ("door_scratch2", 1.0),
  ("hit", 1.0),
  ("pickup", 1.0),
  ("drop", 1.0),
];

let playSound = (name, state, env) =>
  switch (StringMap.find(name, state.sounds)) {
  | (s, volume) => Reprocessing.Env.playSound(s, ~loop=false, ~volume, env)
  | exception Not_found => print_endline("Couldn't find sound " ++ name)
  };

let basedirname = Filename.dirname(Sys.argv[0]) ++ "/";

let loadSounds = env => {
  let loadSoundHelper = (soundMap, (soundName: string, volume)) =>
    StringMap.add(
      soundName,
      (
        Reprocessing.Env.loadSound(
          Printf.sprintf("%ssounds/%s.wav", basedirname, soundName),
          env,
        ),
        volume,
      ),
      soundMap,
    );
  List.fold_left(loadSoundHelper, StringMap.empty, soundNames);
};

let anyKey = (keys, env) =>
  List.exists(k => Reprocessing.Env.key(k, env), keys);

let facingToOffset = dir =>
  switch (dir) {
  | UpD => {x: 0., y: (-0.5)}
  | DownD => {x: 0., y: 0.5}
  | RightD => {x: 0.5, y: 0.}
  | LeftD => {x: (-0.5), y: 0.}
  };

let isCollidable = (x, y, grid: array(array(tileT))) =>
  switch (grid[x][y]) {
  | Blocked
  | Water
  | Fence(_)
  | SeedBin
  | WaterTrough
  | Truck
  | Hay
  | FoodTrough => true
  | _ => false
  };

let handleCollision = (state, prevOffset, offset, pos, grid) => {
  let l = [
    (0, 0),
    (1, 1),
    ((-1), (-1)),
    ((-1), 1),
    ((-1), 0),
    (0, (-1)),
    (1, (-1)),
    (1, 0),
    (0, 1),
  ];
  let padding = 8.;
  let halfTileSizef = tileSizef /. 2.;
  let offset = {
    x:
      Reprocessing.Utils.constrain(
        ~amt=offset.x,
        ~low=-. halfTileSizef,
        ~high=halfTileSizef,
      ),
    y:
      Reprocessing.Utils.constrain(
        ~amt=offset.y,
        ~low=-. halfTileSizef,
        ~high=halfTileSizef,
      ),
  };
  let collided =
    List.exists(
      ((dx, dy)) => {
        let tx = dx + int_of_float((offset.x +. pos.x) /. tileSizef);
        let ty = dy + int_of_float((offset.y +. pos.y) /. tileSizef);
        (
          tx >= Array.length(grid)
          || tx < 0
          || ty >= Array.length(grid[0])
          || ty < 0
        )
        || isCollidable(tx, ty, grid)
        && Reprocessing.Utils.intersectRectRect(
             ~rect1Pos=(
               pos.x +. offset.x +. padding,
               pos.y +. offset.y +. padding,
             ),
             ~rect1W=tileSizef -. padding -. padding,
             ~rect1H=tileSizef -. padding -. padding,
             ~rect2Pos=(
               float_of_int(tx * tileSize),
               float_of_int(ty * tileSize),
             ),
             ~rect2W=tileSizef,
             ~rect2H=tileSizef,
           );
      },
      l,
    );
  /*.Check if anything collides with the barn door */
  let collided =
    if (! collided) {
      let collided =
        Reprocessing.Utils.intersectRectRect(
          ~rect1Pos=(pos.x +. offset.x, pos.y +. offset.y),
          ~rect1W=tileSizef,
          ~rect1H=tileSizef,
          ~rect2Pos=(256., 512.),
          ~rect2W=tileSizef *. 3.,
          ~rect2H=tileSizef -. 8. /* Small offset because otherwise you can kinda get stuck inside the barn door.. .*/
        );
      if (collided) {
        List.exists(
          g =>
            switch (g) {
            | {state: BarnDoor(Closed)} => true
            | _ => false
            },
          state.gameobjects,
        );
      } else {
        false;
      };
    } else {
      true;
    };
  if (collided) {prevOffset} else {offset};
};

let checkIfInBarn = pos =>
  Reprocessing.Utils.intersectRectRect(
    ~rect1Pos=(pos.x +. tileSizef /. 2., pos.y +. tileSizef /. 2.),
    ~rect1W=tileSizef /. 2.,
    ~rect1H=tileSizef /. 2.,
    ~rect2Pos=(5. *. tileSizef, 9. *. tileSizef),
    ~rect2W=256.,
    ~rect2H=256.,
  );

let addChick = gos => [
  {
    pos: {
      x: Reprocessing.Utils.randomf(~min=11., ~max=22.) *. tileSizef,
      y: Reprocessing.Utils.randomf(~min=18., ~max=20.) *. tileSizef,
    },
    action: NoAction,
    state: Chick({
             momentum: {
               x: 0.,
               y: 0.,
             },
             health: 1,
             willDie: false,
           }),
  },
  ...gos,
];
