var Team = (function() {

  // Map from team ID to team objects.
  // Team objects have the property 'victoryPoints'
  var Team = function (tid) {
    this.players = [];
    this.victoryPoints = 0;
  };

  Team.prototype.getVictoryPoints = function () {
    return this.victoryPoints;
  };
  Team.prototype.addVictoryPoints = function (amount) {
    this.victoryPoints += amount;
    return this;
  };

  Team.prototype.addPlayer = function (pid) {
    this.players.push(pid);
    return this;
  };

  Team.prototype.update = function (messages) {
    var vps = 0;
    for (var i = 0; i < messages.length; i++) {
      var message = messages[i];
      if (message.type === MessageTypes.ADD_REQUISITION) {
        var req = must_have_idx(message, 'amount');
        for (var j = 0; j < this.players.length; j++) {
          Players.getPlayer(this.players[j]).addRequisition(req);
        }
      } else {
        invariant_violation(
          'Unknown message of type \'' + message.type + '\' sent to team'
        );
      }
    }

    return this;
  };

  return Team;
})();
