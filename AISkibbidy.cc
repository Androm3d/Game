#include "Player.hh"

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Skibbidy

struct PLAYER_NAME : public Player
{

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player *factory()
  {
    return new PLAYER_NAME;
  }

  /**
   * Types and attributes for your player can be defined here.
   */

  typedef vector<int> VI;
  typedef vector<vector<char>> VVC;
  typedef vector<vector<bool>> VVB;
  typedef vector<vector<int>> VVI;

  map<int, int> kind; // For pioneers: 0 -> random, 1 -> cyclic.

  // set<Pos> scanned;

  struct unit_move
  {
    int id;
    int priority;
    Dir next;
  };

  class Compare
  {
  public:
    bool operator()(unit_move a, unit_move b)
    {
      return a.priority > b.priority;
    }
  };

  priority_queue<unit_move, vector<unit_move>, Compare> order;

  Dir dodge(int d, Pos p) // also chase
  {
    cerr << endl << "dodge" << endl;
    int i = 0;
    int scanned = 0;
    if (p.k == 0)
    {
      while (cell(p + Dir(((d + i) % 8))).type == Rock or cell(p + Dir(((d + i) % 8))).id != -1)
      {
        i *= -1;
        if (i < 1)
          --i;
        ++scanned;
        if (scanned == 8)
          return None;
      }
      cerr << "dodge1 dir: " << Dir((d + i) % 8) << endl;
      return Dir((d + i) % 8);
    }
    else
    {
      cerr << endl
           << "dodge up" << endl;
      if (d == 1)
        return RT;
      else if (d == 2)
      {
        if (random(0, 1))
          return RT;
        else
          return BR;
      }
      else if (d == 3)
        return BR;
      else
        return Right;
    }
  }

  Dir BestMove(Pos p0)
  {
    cerr << endl << "Best move" << endl << endl;
    if (p0.k == 0)
    {
      int max = -1;
      int best = -1;
      int k = random(0, 8);
      for (int i = 0; i < 8; ++i)
      {
        k = (k + i) % 8;
        Cell c = cell(p0 + Dir(k));
        if (c.type != Rock and c.id == -1)
        {
          if (c.owner != -1)
          {
            if (c.owner == me())
            {
              if (best == -1)
                best = k;
            }
            else
            {
              int points = nb_cells(c.owner) + (30 * nb_gems(c.owner));
              if (points >= max or best == -1)
              {
                best = k;
                max = points;
              }
            }
          }
          else if (best == -1 or max == -1)
          {
            best = k;
            max = 0;
          }
        }
      }
      return Dir(best);
    }
    else
    {
      cerr << endl << "Best move up" << endl;
      queue<Pos> q;
      q.push(p0);
      VVI dist(40, VI(80, -1));
      dist[p0.i][p0.j] = 0;
      while (not q.empty())
      {
        Pos p = q.front();
        q.pop();
        if (dist[p.i][p.j] == 4)
          return Right;
        int k = random(0, 8);
        for (int i = 0; i < 8; ++i)
        {
          k = (k + i) % 8;
          Pos pp = p + Dir(k);
          if (pos_ok(pp))
          {
            if (cell(pp).gem)
              return Dir(k);
            // if (cell(pp).type == Elevator) return Dir(k);
            if (dist[pp.i][pp.j] == -1)
            {
              q.push(pp);
              dist[pp.i][pp.j] = 1 + dist[p.i][p.j];
            }
          }
        }
      }
      return Right;
    }
  }

  void nextMove_down(Pos p0, int id)
  {
    cerr << endl << "next move down" << endl << endl;
    queue<Pos> q;
    q.push(p0);
    VVI dist(40, VI(80, -1));
    dist[p0.i][p0.j] = 0;
    while (not q.empty())
    {
      Pos p = q.front();
      q.pop();
      if (unit(id).type == Pioneer and dist[p.i][p.j] == 4)
      {
        order.push({id, 2, BestMove(p0)});
        return;
      }
      int k = 0;
      bool next = true;
      while (next)
      {
        Pos pp = p + Dir(k);
        if (dist[pp.i][pp.j] == -1 and cell(pp).type != Rock)
        { 
          q.push(pp);
          dist[pp.i][pp.j] = 1 + dist[p.i][p.j];
        }
        if (cell(pp).id != -1)
        {
          cerr << "different unit spotted" << endl;
          int id2 = cell(pp).id;
          if (unit(id2).type == Hellhound and dist[pp.i][pp.j] <= 4)
          {
            // always run away
            cerr << "Hellhound spotted at dir: " << Dir(k) << endl;
            cerr << "dodge called with dir: " << (k + 4) % 8 << endl;
            order.push({id, 0, dodge((k + 4) % 8, p0)});
            cerr << "order placed" << endl;
            return;
          }
          else if (unit(id2).player != me())
          {
            // our unit pioneer -> run away
            if (unit(id).type == Pioneer and unit(id2).type != Pioneer and dist[pp.i][pp.j] <= 3)
            {
              cerr << "Ally Pioneer spotted Enemy Furyan at dir: " << Dir(k) << endl;
              cerr << "dodge called with dir: " << Dir((k + 4) % 8) << endl;
              order.push({id, 1, dodge((k + 4) % 8, p0)});
              cerr << "order placed" << endl;
              return;
            }
            if (unit(id).type == Furyan)
            {
              cerr << "Ally Furian spotted Enemy unit at dir: " << Dir(k) << endl;
              cerr << "dodge called with dir: " << Dir(k) << endl;
              if (unit(id2).type == Pioneer or unit(id).health > unit(id2).health) 
              { 
                // only if our furian has more health he attacks
                if (dist[pp.i][pp.j] > 1) order.push({id, 2, dodge(k, p0)});
                else order.push({id, 0, Dir(k)});
                return;
              }
              else
              {
                // otherwise it runs
                if (dist[pp.i][pp.j] > 1) order.push({id, 2, dodge((k + 4) % 8, p0)});
                else order.push({id, 0, dodge((k + 4) % 8, p0)});
              }
            }
          }
        }
        next = k != 7;
        k += 2;
        if (k != 7) k %= 7;
      }
    }
  }

  void nextMove_up(Pos p0, int id)
  {
    cerr << endl << "next move up" << endl;
    queue<Pos> q;
    q.push(p0);
    VVI dist(40, VI(80, -1));
    dist[p0.i][p0.j] = 0;
    while (not q.empty())
    {
      cerr << endl << "next move up1" << endl;
      Pos p = q.front();
      q.pop();
      if (dist[p.i][p.j] == 3)
      {
        order.push({id, 2, BestMove(p0)});
        return;
      }
      for (int i = 0; i < 8; ++i)
      {
        int k = (random(0, 8) + i) % 8;
        Pos pp = p + Dir(k);
        if (pos_ok(pp))
        {
          if (dist[pp.i][pp.j] == -1)
          { // and not scanned.count(pp)) {
            // scanned.insert(pp);
            cerr << endl << "next move up2" << endl;
            q.push(pp);
            dist[pp.i][pp.j] = 1 + dist[p.i][p.j];
          }
          if (cell(pp).id != 1 and dist[pp.i][pp.j] < 3)
          {
            int id2 = cell(pp).id;
            if (unit(id2).type == Necromonger and dist[pp.i][pp.j] < 3)
            {
              order.push({id, 1, dodge((k + 4) % 8, p0)});
              return;
            }
          }
        }
      }
    }
  }

  int get_sun_distance(Pos p)
  {
    int i = 0;
    p.k = 1;
    while (not daylight(p) and i < 8)
    {
      cerr << endl << "get sun distance" << endl;
      p += Left;
      ++i;
    }
    return i;
  }

  void move_furyans()
  {
    VI F = furyans(me());
    for (int id : F)
    {
      Pos p = unit(id).pos;
      nextMove_down(p, id);
    }
  }

  void move_pioneers()
  {
    VI P = pioneers(me());
    for (int id : P)
    {
      Pos p = unit(id).pos;
      /*
      cerr << unit(id) << endl; // You can print to cerr to debug.
      if (kind.find(id) == kind.end()) kind[id] = random(0, 1);
      if (kind[id] == 0) command(id, Dir(random(0, 7)));
      else command(id, Dir(2*(round()%4)));
      */
      if (p.k == 0)
      {
        if (cell(p).type == Elevator and get_sun_distance(unit(id).pos) > 6 and round() > 100)
          order.push({id, 2, Up});
        else
          nextMove_down(p, id);
      }
      else
      {
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
  virtual void play()
  {
    move_pioneers();
    move_furyans();
    while (not order.empty())
    {
      if (order.top().priority < 2)
        cerr << "dodge or chase commanded dir: " << order.top().next << endl << endl;
      else
        cerr << "best move (not dodge) commanded" << endl << endl;
      command(order.top().id, order.top().next);
      order.pop();
    }
    // scanned.clear();
  }
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
