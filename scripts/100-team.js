var Teams = (function() {

  // External API
  var exports = {};

  // Map from team ID to team objects.
  // Team objects have the property 'victoryPoints'
  var teams = {};

  var Team = function (tid) {
    var players = [];
    var victoryPoints = 0;

    this.getVictoryPoints = function () {
      return victoryPoints;
    }

    this.addPlayer = function (pid) {
      players.push(pid);
      return this;
    };

    this.update = function (messages) {
      var vps = 0;
      for (var i = 0; i < messages.length; i++) {
        var message = messages[i];
        if (message.type === MessageTypes.ADD_VPS) {
          vps += must_have_idx(message, 'amount');
        } else if (message.type === MessageTypes.ADD_REQUISITION) {
          var req = must_have_idx(message, 'amount');
          for (var i = 0; i < players.length; i++) {
            Players.getPlayer(players[i]).addRequisition(req);
          }
        } else {
          invariant_violation(
            'Unknown message of type \'' + message.type + '\' sent to team'
          );
        }
      }

      victoryPoints += vps;
    };
  };

  var verifyTeam = function (tid) {
    invariant(
      teams[tid] !== undefined, 
      "Teams API: Must provide a valid team ID for Teams API calls"
    );
  };

  exports.getVictoryPoints = function() {
    var ret = [];
    for (var tid in teams) {
      ret.push({
        tid: tid,
        vps: teams[tid].getVictoryPoints(),
      });
    }
    return ret;
  };


  // Create a new team, with zero victory points.
  exports.addTeam = function(tid) {
    if (tid in teams) {
      return;
    }
    teams[tid] = new Team(tid);
  };
  exports.addPlayer = function(tid, pid) {
    this.addTeam(tid);
    verifyTeam(tid);
    teams[tid].addPlayer(pid);
  };

  // @param tid the team getting the points
  // @param amount how much gained or lost
  // @param entity (victory point) we're receiving it from
  exports.addVPs = function(tid, amount, from_eid) {
    verifyTeam(tid);
    teams[tid].addVictoryPoints(amount, from_eid);
  };

  exports.addRequisition = function(tid, amount, from_eid) {
    verifyTeam(tid);
    teams[tid].addRequisition(amount, from_eid);
  };

  exports.getTeams = function () {
    return teams;
  };

  return exports;
})();
