var Teams = (function() {

  // External API
  var exports = {};

  // Map from team ID to team objects.
  // Team objects have the property 'victoryPoints'
  var teams = {};

  var Team = function (tid) {
    this.tid_ = tid;
    this.players_ = [];
    this.victoryPoints_ = 0;

    this.getVictoryPoints = function () {
      return this.victoryPoints_;
    }

    this.addVictoryPoints = function (vps, from_eid) {
      this.victoryPoints_ += vps;
      return this;
    };
    this.addRequisition = function (req, source_id) {
      for (var i = 0; i < this.players_.length; i++) {
        Players.getPlayer(this.players_[i]).addRequisition(req);
      }
    };
    this.addPlayer = function (pid) {
      this.players_.push(pid);
      return this;
    };
  };

  var verifyTeam = function (tid) {
    invariant(
      teams[tid] !== undefined, 
      "Teams API: Must provide a valid team ID for Teams API calls"
    );
  };

  exports.getVictoryPoints = function(tid) {
    verifyTeam(tid);
    return teams[tid].getVictoryPoints();
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

  return exports;
})();
