Module['preRun'].push(function() {
  addRunDependency('syncfs');
  FS.mount(IDBFS, {}, '/home/web_user');
  FS.syncfs(true, function (err) {
    if (err) throw err;
    removeRunDependency('syncfs');
  });
});

Module['syncFS'] = function() {
  FS.syncfs(false, function (err) {
    if (err) throw err;
  });
}
