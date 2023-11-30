#include "Player.hh"


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME SkibbidyToilet


struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }

  /**
   * Types and attributes for your player can be defined here.
   */



  typedef vector<int> VI;
  typedef vector<vector<char>> VVC;
  typedef vector<vector<int>> VVI;

  map<int, int> kind; // For pioneers: 0 -> random, 1 -> cyclic.

  set<Pos> danger;

  struct unit_move {
    int id;
    int priority;
    Dir next;
  };

  class Compare
  {
  public:
      bool operator() (unit_move a, unit_move b)
      {
          return a.priority < b.priority;
      }
  };

  priority_queue<unit_move, vector<unit_move>, Compare> order;
  
  Dir dodge(int d, Pos p) 
  {
    int i = 0;
    if (p.k == 0) {
      while (cell(p+Dir((d + i)%8)).type == Rock) {
        i*=-1;
        if (i < 1) --i;
      }
      return Dir(d + i);
    }
    else {
      if (d == 1) return RT;
      else if (d == 2) {
        if (random(0, 1)) return RT;
        else return BR;
      }
      else if (d == 3) return BR;
      else return Right;
    }
  }

  int BestMove(Pos p0)
  {
    if (p0.k == 0) {
      int max = 0;
      int best = -1;
      for(int i = 0; i<8; ++i) {
        Cell c = cell(p0 + Dir(i));
        if (c.type != Rock and c.id == -1) {
          if (c.owner != -1) {
            int points = nb_cells(c.owner) + (30* nb_gems(c.owner));
            if (points >= max) {
              best = i;
              max = points;
            }
            else if (best == -1) best = i;
          }
          else if (best == -1) best = i;
        }
      }
      return best;
    }
    else {
    queue<Pos> q;
    q.push(p0);
    VVI dist(10, vector<int>(10, -1));
    dist[p0.i][p0.j] = 0;
    while (not q.empty()) {
        Pos p = q.front();
        q.pop();
        if (dist[p.i][p.j] == 4) return 2;
        int k = 0;
        bool next = true;
        while (next) {
            Pos pp = p + Dir(k);
            if (cell(pp).gem) return k;
            if (dist[pp.i][pp.j] == -1) {
                q.push(pp);
                dist[pp.i][pp.j] = 1 + dist[p.i][p.j];
            }
            next = k != 7;
            k += 2;
            if (k != 5) k %= 7;
        }
      }
      return 2;
    }
  }

  void nextMove_down(Pos p0, int id)
  {   
    queue<Pos> q;
    q.push(p0);
    VVI dist(10, vector<int>(10, -1));
    dist[p0.i][p0.j] = 0;
    while (not q.empty()) {
        Pos p = q.front();
        q.pop();
        if (dist[p.i][p.j] == 3) order.push({id, 2, Dir(BestMove(p0))});
        int k = 0;
        bool next = true;
        while (next) {  //check straight before corners
            Pos pp = p + Dir(k);
            if (dist[pp.i][pp.j] == -1 and cell(pp).type != Rock) {
                q.push(pp);
                dist[pp.i][pp.j] = 1 + dist[p.i][p.j];
            }
            if (cell(pp).id != 1){
                int id = cell(pp).id;
                if (unit(id).type != Pioneer and dist[pp.i][pp.j] < 3) {
                  danger.insert(p);
                  order.push({id, dist[pp.i][pp.j]-1, dodge((k+4)%8, p0)});
                }
                if (unit(id).type == Hellhound and dist[pp.i][pp.j] == 3) {
                  danger.insert(p);
                  order.push({id, dist[pp.i][pp.j]-1, dodge((k+4)%8, p0)});
                }
            }
            next = k != 7;
            k += 2;
            if (k != 5) k %= 7;
        }
    }
  }

  void nextMove_up(Pos p0, int id) {
    queue<Pos> q;
    q.push(p0);
    VVI dist(10, vector<int>(10, -1));
    dist[p0.i][p0.j] = 0;
    while (not q.empty()) {
        Pos p = q.front();
        q.pop();
        if (dist[p.i][p.j] == 2) order.push({id, 2, Dir(BestMove(p0))});
        int k = 0;
        bool next = true;
        while(next) {
            Pos pp = p + Dir(k);
            if (dist[pp.i][pp.j] == -1) {
                q.push(pp);
                dist[pp.i][pp.j] = 1 + dist[p.i][p.j];
            }
            if (cell(pp).id != 1){
                int id = cell(pp).id;
                if (unit(id).type == Necromonger and dist[pp.i][pp.j] < 3) {
                  danger.insert(p);
                  order.push({id, 1, dodge((k+4)%8, p0)});
                }
            }
            next = k != 7;
            k += 2;
            if (k != 5) k %= 7;
        }
    }
  }

  int get_sun_distance(Pos p)
  {
    int i = 0;
    while(not daylight(p)) {
      p += Left;
      ++i;
    }
    return i;
  }

  void move_furyans() {
    VI F = furyans(me());
    int n = F.size();
    VI perm = random_permutation(n);
    for (int i = 0; i < n; ++i) {
      // id is an own furyan. For some reason (or not) we treat our furyans in random order.
      int id = F[perm[i]];
      if (random(0, 2) == 0) command(id, Dir(random(0, 9))); // With probability 1/3, we move at random.
      else { // Otherwise..
        bool enemy = false;
        for (int k = 0; not enemy and k < 8; ++k) {
          Pos p = unit(id).pos;
          if (pos_ok(p)) {
            int id2 = cell(p).id;
            if (id2 != -1 and unit(id2).player != me()) { // if we are next to an enemy, we attack.
              enemy = true;
              command(id, Dir(k));
            }
          }
        }
        // Finally, the following code does several things, most of them stupid.
        // It's just to show that there are many possibilities.
        if (not enemy) {
          if (cell(unit(id).pos).type == Elevator) command(id, Up);
          else if (round() < 40) command(id, Left);
          else if (round() > 180) command(id, None);
          else if (random(0, 1)) {
            set<Pos> S;
            while ((int)S.size() < 4) S.insert(Pos(random(0, 39), random(0, 79), 0));
            vector<Pos> V(S.begin(), S.end());
            if (V[random(0, 3)].i >= 30 ) command(id, Bottom);
            else command(id, RT);
          }
          else if (status(0) > 0.8) command(id, Left);
          else if (unit(id).health < 20) command(id, Dir(2*random(0, 3)));
          else if (cell(10, 20, 0).owner == 2) command(id, TL);
          else if (nb_cells(3) > 50) command(id, LB);
          else if (nb_gems(me()) < 4) command(id, BR);
          else if (cell(Pos(20, 30, 0)).type == Cave) command(id, Bottom);
          else if (cell(Pos(2, 2, 1)).gem) command(id, Top);
          else if (daylight(Pos(0, 0, 1))) command(id, RT);
          else cerr << unit(id).pos << endl; // You can print to cerr to debug.
        }
      }
    }
  }

  void move_pioneers() {
    vector<int> P = pioneers(me());
    for (int id : P) {
      Pos p = unit(id).pos;
      /*
      cerr << unit(id) << endl; // You can print to cerr to debug.
      if (kind.find(id) == kind.end()) kind[id] = random(0, 1);
      if (kind[id] == 0) command(id, Dir(random(0, 7)));
      else command(id, Dir(2*(round()%4)));
      */
      if(p.k == 0) {
        if (cell(p).type == Elevator and get_sun_distance(unit(id).pos) > 6)
          order.push({id, 2, Up});
        else
          nextMove_down(p, id);
      }
      else {
        if (cell(p).type == Elevator and get_sun_distance(unit(id).pos) < 7) 
          order.push({id, 2, Down});
        else 
          nextMove_up(p, id);
      }
    }
  }


  /**
   * Play method, invoked once per each round.
   */
  virtual void play () {
    move_pioneers();
    move_furyans();
    while(not order.empty()) {
      command(order.top().id, order.top().next);
      order.pop();
    }
  }

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
