Teams = (function() {

  // External API
  var TeamsAPI = {};

  // Map from team ID to team objects.
  // Team objects have the property 'victoryPoints'
  var teams = {};

  function verifyTeam(tid) {
    invariant(teams[tid] !== undefined, 
      "Teams API: Must provide a valid team ID for Teams API calls");
  }

  TeamsAPI.getVictoryPoints = function(tid) {
    verifyTeam(tid);
    return teams[tid].victoryPoints;
  };


  TeamsAPI.setVictoryPoints = function(tid, numPoints) {
    verifyTeam(tid);
    invariant(typeof numPoints === "number", 
        "Must set victory points to a number.");
    teams[tid].victoryPoints = numPoints;
  };

  // Create a new team, with zero victory points.
  TeamsAPI.addTeam = function(tid) {
    teams[tid] = {victoryPoints : 0};
  };

  // @param tid the team getting the points
  // @param amount how much gained or lost
  // @param entity (victory point) we're receiving it from
  TeamsAPI.addVPs = function(tid, amount, eidFrom) {
    teams[tid].victoryPoints += amount;
  };

  return TeamsAPI;
})();
