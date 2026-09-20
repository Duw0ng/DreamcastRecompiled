(() => {
  const C = window.DCR_CONFIG || {owner:'Duw0ng', repo:'DreamcastRecompiled', branch:'main', projectName:'Dreamcast Recompiled', releaseFallback:'v0.1 Official'};
  const API = `https://api.github.com/repos/${C.owner}/${C.repo}`;
  const REPO_URL = `https://github.com/${C.owner}/${C.repo}`;

  const $ = (sel, root=document) => root.querySelector(sel);
  const $$ = (sel, root=document) => [...root.querySelectorAll(sel)];
  const esc = (v='') => String(v).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#039;'}[c]));

  function wireNav(){
    const btn = $('#menuBtn'), links = $('#navLinks');
    btn?.addEventListener('click', () => links.classList.toggle('open'));
    $$('[data-repo-link]').forEach(a => a.href = REPO_URL);
    $$('[data-issues-link]').forEach(a => a.href = `${REPO_URL}/issues`);
    $$('[data-new-report]').forEach(a => a.href = `${REPO_URL}/issues/new?template=compatibility-report.yml`);
  }

  async function gh(path){
    const res = await fetch(`${API}${path}`, {headers:{Accept:'application/vnd.github+json'}});
    if(!res.ok) throw new Error(`GitHub ${res.status}`);
    return res.json();
  }

  function relTime(date){
    if(!date) return '—';
    const delta = Math.round((new Date(date)-Date.now())/1000);
    const units = [[31536000,'year'],[2592000,'month'],[86400,'day'],[3600,'hour'],[60,'minute']];
    const rtf = new Intl.RelativeTimeFormat(undefined,{numeric:'auto'});
    for(const [sec,unit] of units) if(Math.abs(delta)>=sec) return rtf.format(Math.round(delta/sec),unit);
    return rtf.format(delta,'second');
  }

  async function loadGitHub(){
    const star = $$('[data-gh-stars]'), fork = $$('[data-gh-forks]'), issue = $$('[data-gh-issues]');
    try {
      const repo = await gh('');
      star.forEach(e => e.textContent = repo.stargazers_count.toLocaleString());
      fork.forEach(e => e.textContent = repo.forks_count.toLocaleString());
      issue.forEach(e => e.textContent = repo.open_issues_count.toLocaleString());
      $$('[data-gh-branch]').forEach(e => e.textContent = repo.default_branch);
      $$('[data-gh-updated]').forEach(e => e.textContent = relTime(repo.pushed_at));
    } catch(err) {
      console.info('GitHub live metadata unavailable; using static fallback.', err.message);
    }

    try {
      const releases = await gh('/releases?per_page=1');
      const rel = releases[0];
      if(rel){
        $$('[data-release-name]').forEach(e => e.textContent = rel.name || rel.tag_name);
        $$('[data-release-date]').forEach(e => e.textContent = new Date(rel.published_at || rel.created_at).toLocaleDateString());
        $$('[data-release-download]').forEach(a => a.href = rel.html_url);
        const asset = rel.assets?.[0];
        if(asset) $$('[data-release-asset]').forEach(e => e.textContent = asset.name);
      }
    } catch(err) { console.info('No public release data yet.'); }

    const commitBox = $('#commitList');
    if(commitBox){
      try {
        const commits = await gh('/commits?per_page=5');
        commitBox.innerHTML = commits.map(c => `
          <a class="commit" href="${esc(c.html_url)}" target="_blank" rel="noreferrer">
            <code>${esc(c.sha.slice(0,7))}</code>
            <p>${esc((c.commit?.message || '').split('\n')[0])}</p>
            <time>${esc(relTime(c.commit?.author?.date))}</time>
          </a>`).join('');
      } catch(err) {
        commitBox.innerHTML = '<div class="notice">Recent commits will appear here automatically once the repository is public.</div>';
      }
    }
  }

  const statusOrder = ['Playable','Ingame','Menu','Intro','Bootable','Nothing','Untested'];
  const statusClass = s => (s || 'Untested').toLowerCase().replace(/\s+/g,'-');
  const stateSymbol = s => s === 'Working' ? '✓' : s === 'Partial' ? '~' : s === 'Broken' ? '×' : '·';
  const stateClass = s => `state-${(s||'Untested').toLowerCase()}`;

  async function compatibilitySource(){
    // Prefer the copy shipped with the site. On GitHub Pages this is already repo-backed.
    const res = await fetch('./data/compatibility.json', {cache:'no-store'});
    if(!res.ok) throw new Error('Compatibility data unavailable');
    return res.json();
  }

  async function initCompatibility(){
    const table = $('#compatTable');
    if(!table) return;
    const body = $('tbody', table);
    const search = $('#gameSearch'), status = $('#statusFilter'), priority = $('#priorityFilter');
    const resultCount = $('#resultCount'), pageText = $('#pageText');
    const prev = $('#prevPage'), next = $('#nextPage');
    const PAGE = 30;
    let page = 1, all = [], filtered = [];

    try {
      const data = await compatibilitySource();
      all = data.games || [];
    } catch(err) {
      body.innerHTML = `<tr><td colspan="9"><div class="notice">Could not load compatibility data.</div></td></tr>`;
      return;
    }

    function counts(){
      const map = Object.fromEntries(statusOrder.map(s => [s,0]));
      all.forEach(g => map[g.status] = (map[g.status] || 0) + 1);
      $$('[data-status-count]').forEach(el => el.textContent = map[el.dataset.statusCount] || 0);
      $$('[data-total-games]').forEach(el => el.textContent = all.length);
      $$('[data-status-filter-card]').forEach(el => el.addEventListener('click', () => {
        status.value = el.dataset.statusFilterCard;
        page = 1; render();
        document.querySelector('.compat-toolbar')?.scrollIntoView({behavior:'smooth',block:'center'});
      }));
    }

    function pass(g){
      const q = search.value.trim().toLowerCase();
      return (!q || [g.title,g.group,g.notes,g.version].some(v => String(v||'').toLowerCase().includes(q))) &&
        (!status.value || g.status === status.value) &&
        (!priority.value || g.priority === priority.value);
    }

    function stateCell(v,title){ return `<span class="state ${stateClass(v)}" title="${esc(title)}: ${esc(v)}">${stateSymbol(v)}</span>`; }

    function render(){
      filtered = all.filter(pass);
      const pages = Math.max(1, Math.ceil(filtered.length/PAGE));
      page = Math.min(page,pages);
      const slice = filtered.slice((page-1)*PAGE,page*PAGE);
      body.innerHTML = slice.map((g,i) => `
        <tr data-index="${all.indexOf(g)}">
          <td class="game-name">${esc(g.title)}<span class="mini">${esc(g.group || 'Dreamcast title')}</span></td>
          <td><span class="pill ${statusClass(g.status)}">${esc(g.status)}</span></td>
          <td>${stateCell(g.boot,'Boot')}</td>
          <td>${stateCell(g.menus,'Menus')}</td>
          <td>${stateCell(g.graphics,'Graphics')}</td>
          <td>${stateCell(g.audio,'Audio')}</td>
          <td>${stateCell(g.controls,'Controls')}</td>
          <td>${stateCell(g.gameplay,'Gameplay')}</td>
          <td>${esc(g.version || '—')}</td>
        </tr>`).join('') || '<tr><td colspan="9">No games match these filters.</td></tr>';
      resultCount.textContent = `${filtered.length} game${filtered.length===1?'':'s'}`;
      pageText.textContent = `Page ${page} of ${pages}`;
      prev.disabled = page <= 1; next.disabled = page >= pages;
      $$('tbody tr[data-index]',table).forEach(row => row.addEventListener('click', () => openGame(all[+row.dataset.index])));
    }

    function openGame(g){
      const modal = $('#gameModal');
      $('#modalStatus').className = `pill ${statusClass(g.status)}`;
      $('#modalStatus').textContent = g.status;
      $('#modalTitle').textContent = g.title;
      $('#modalVersion').textContent = g.version || 'Not tested';
      $('#modalPriority').textContent = g.priority || '—';
      $('#modalGroup').textContent = g.group || '—';
      $('#modalNotes').textContent = g.notes || 'No validation notes yet.';
      const checks = [['Boot',g.boot],['Menus',g.menus],['Graphics',g.graphics],['Audio',g.audio],['Controls',g.controls],['VMU / Save',g.vmu],['Gameplay',g.gameplay],['FPS',g.fps || '—']];
      $('#modalChecks').innerHTML = checks.map(([k,v]) => `<div class="detail-cell"><span>${esc(k)}</span><strong>${esc(v || 'Untested')}</strong></div>`).join('');
      const q = encodeURIComponent(`is:issue compatibility "${g.title}"`);
      $('#modalReport').href = `${REPO_URL}/issues?q=${q}`;
      $('#modalNewReport').href = `${REPO_URL}/issues/new?template=compatibility-report.yml&title=${encodeURIComponent(`[Compatibility] ${g.title}`)}`;
      modal.classList.add('open');
      document.body.style.overflow='hidden';
    }

    function closeModal(){ $('#gameModal').classList.remove('open'); document.body.style.overflow=''; }
    $('#modalClose').addEventListener('click',closeModal);
    $('#gameModal').addEventListener('click',e => { if(e.target.id==='gameModal') closeModal(); });
    document.addEventListener('keydown',e => { if(e.key==='Escape') closeModal(); });
    [search,status,priority].forEach(el => el.addEventListener(el===search?'input':'change',() => {page=1;render();}));
    prev.addEventListener('click',() => {page--;render(); table.scrollIntoView({behavior:'smooth'});});
    next.addEventListener('click',() => {page++;render(); table.scrollIntoView({behavior:'smooth'});});
    const initialStatus = new URLSearchParams(location.search).get('status');
    if(initialStatus && statusOrder.includes(initialStatus)) status.value = initialStatus;
    counts(); render();
  }

  async function initHomeCompat(){
    if(!$('#homeCompatibility')) return;
    try {
      const d = await compatibilitySource();
      const counts = {};
      (d.games||[]).forEach(g => counts[g.status]=(counts[g.status]||0)+1);
      $$('[data-home-status]').forEach(e => e.textContent = counts[e.dataset.homeStatus] || 0);
      $$('[data-total-games]').forEach(e => e.textContent = d.games.length);
    } catch(_){}
  }

  wireNav();
  loadGitHub();
  initCompatibility();
  initHomeCompat();
})();
