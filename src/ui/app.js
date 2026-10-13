/* RE4R ShapeKey Studio 0.4 UI (Claude, 2026-10-12). Talks to host_webview.c; falls back to a demo engine in a plain browser.
   Snapshot rows (pipe v1): S token owned source strength status profile capture error compare sweep
                            R available running port ip packets frames age rejected error status
                            K id label region manual actual writable mesh group      M index name channels wc wl limit ready */
'use strict';
const HOST = window.chrome && window.chrome.webview;
const Q = new URLSearchParams(location.search);
const DEMO = !HOST || Q.get('demo') === '1';
const RM = matchMedia('(prefers-reduced-motion: reduce)');
const $ = (s, r = document) => r.querySelector(s);
const el = (tag, cls, html) => { const e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; };
const icon = id => `<svg viewBox="0 0 24 24"><use href="#i-${id}"/></svg>`;
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
const fmt = v => (Math.round(v * 100) / 100).toFixed(2);
const store = { get(k, d) { try { const v = localStorage.getItem('sks.' + k); return v == null ? d : v; } catch { return d; } }, set(k, v) { try { localStorage.setItem('sks.' + k, v); } catch {} } };

/* ------------------------------------------------------------------ i18n */
const T = {
  zh: {
    app_title: 'RE4R 形态键工作台', app_sub: 'RE4 重制版角色形态键编辑', step_dir: '游戏目录', step_install: '安装桥接', step_capture: '面捕连接',
    setup_title: '先连接你的游戏', setup_lead: '设置 RE4 目录，准备形态键与面捕控制。', dir_title: '游戏目录', dir_hint: '选择直接包含 re4.exe 的文件夹',
    browse: '浏览文件夹', validate: '验证目录', install_title: '安装桥接', install_hint: '安装前请退出 RE4；当前脚本会自动备份。\n已安装过可以直接启动游戏连接。',
    install_btn: '安装 / 更新桥接与面捕', installing_btn: '正在安装…', enter: '进入工作台',
    dir_ok: '已找到 re4.exe，可以进入工作台。', dir_unsaved: '目录有效，但设置无法保存；请把程序放到可写文件夹。', dir_bad: '请选择直接包含 re4.exe 的游戏文件夹。',
    dir_none: '还没有设置游戏目录。', installing: '正在安装桥接，原脚本会先备份…', installed: '桥接已安装。启动 / 重启 RE4 后即可连接。',
    install_failed: '安装失败：请先退出游戏；原因见 installer.log。', install_nostart: '无法启动安装器，请查看使用说明。', log_unwritable: '无法写入 installer.log，请使用可写文件夹。',
    dir_found: '已找到 re4.exe', dir_wait: '目录未验证，工作台暂未开放',
    source: '表情来源', strength: '强度', start: '开始控制', release: '释放控制', theme: '切换深浅色', topmost: '窗口置顶 / Always on top (Ctrl+T)', t_top_on: '已置顶', t_top_off: '已取消置顶',
    connected: '已连接到游戏', waiting: '等待模型绑定', offline: '未连接', demo_conn: '演示模式', conn_sub_on: 'Resident Evil 4 Remake', conn_sub_off: '启动游戏并加载桥接后自动连接',
    nav_face: '表情控制', nav_face_s: '面部表情与情绪', nav_shape: '面部雕刻', nav_shape_s: '五官与面部轮廓', nav_body: '身体与服装', nav_body_s: '体型与服装长度',
    nav_hair: '发型与耳朵', nav_hair_s: '刘海与兽耳', nav_preset: '预设方案', nav_preset_s: '保存与载入预设', nav_capture: 'RhyLive 面捕', nav_capture_s: '手机面捕接收',
    nav_diag: '诊断工具', nav_diag_s: '连接状态与网格检查', nav_setup: '目录与初始化', nav_setup_s: '游戏目录与桥接安装',
    sub_face: '调整角色的面部表情，实时作用到游戏内。', sub_shape: '调整脸型与五官比例。', sub_body: '调整体型与服装长度，衣服会随身体一起变化。', sub_hair: '调整刘海与兽耳。',
    sub_preset: '把当前所有滑条存成预设，或从文件载入。', sub_capture: '用手机 RhyLive 实时驱动表情。', sub_diag: '连接、网格权重数组与运行状态。',
    nav_outfit_variant: '服装变体', nav_outfit_variant_s: '换装款式与细节', nav_nail: '指甲', nav_nail_s: '指甲款式与长度', nav_eye_detail: '眼部细节', nav_eye_detail_s: '瞳孔与高光形状',
    nav_mouth_detail: '口部细节', nav_mouth_detail_s: '舌头与牙齿', nav_other: '其他', nav_other_s: '未分类形态键', nav_auto_s: '自动识别的分区',
    sub_outfit_variant: '切换服装的款式与细节变体。', sub_nail: '调整指甲的款式与长度。', sub_eye_detail: '调整瞳孔、高光等眼部细节形状。', sub_mouth_detail: '调整舌头、牙齿等口腔细节。',
    sub_other: '没有归入任何分区的形态键。', sub_auto: '此分区由模型自动提供。',
    compare: '中性对比', sweep: '逐键巡检', zero_page: '重置本页', copy: '复制参数', paste: '粘贴参数', search: '搜索形态键…', keys: '个形态键',
    need_control: '点“开始控制”后即可调整', no_keys: '当前模型没有这一类形态键', no_keys_sub: '身体、头发需要在 Fluffy 启用模型的对应模块',
    sel_title: '当前选中参数', sel_none: '点选任意一行查看详情', p_name: '参数名称', p_group: '所在分组', p_manual: '当前值（本地）', p_actual: '游戏内读取值', p_range: '数值范围', p_mesh: '所在网格', p_note: '参数说明',
    reset_one: '重置此参数', locate: '定位到该行', mesh_head: '头部', mesh_body: '身体', mesh_hair: '头发',
    why_alloc: '权重数组未分配', alloc: '分配', why_unbound: '模块未启用',
    recv_title: 'RhyLive 面捕', recv_hint: '通过游戏内接收器读取 ARKit 52 通道', udp: 'UDP 端口', lan: '手机目标地址（本机局域网 IPv4）', lan_none: '未找到局域网 IPv4',
    lan_note: '手机端口与上方一致；0.0.0.0 只用于电脑监听。', recv_start: '启动 / 重启接收', recv_stop: '停止接收', recv_save: '保存接收设置',
    listening: '监听中', stopped: '已停止', recv_wait: '等待游戏启动并加载桥接', packets: '数据包', frames: '面部帧', age: '数据间隔',
    st_off: '未启动', st_wait: '等待手机数据', st_rx: '接收中', st_lost: '已中断', st_lost_for: '已中断 {s} s', st_lost_cap: '已中断 >60 s',
    fps_lbl: '面部帧率', pps_lbl: '数据包', total: '累计', rejected: '已丢弃', spark_old: '60 秒前', spark_now: '现在', demo_stream: '演示：手机推流',
    rt_title: '响应速度 / 实时度', rt_smooth: '平滑', rt_bal: '均衡', rt_live: '实时', rt_hint: '越高越跟手，越低越平滑（抖动少）。', rt_na: '需要更新桥接后可用',
    preset_save: '保存预设', preset_load: '载入预设', preset_bad: '预设不属于当前模型，未载入。', preset_ok: '预设已载入', copied: '已复制本页参数', pasted: '已粘贴参数',
    paste_bad: '剪贴板里没有可用的参数', nonzero: '当前非零的键',
    d_conn: '连接', d_online: '管道', d_token: '绑定标识', d_status: '面板状态', d_capture: '面捕状态', d_error: '最近错误', d_meshes: '网格权重数组', d_host: '程序',
    d_ready: '已就绪', d_notready: '未分配', d_version: '版本', d_dir: '游戏目录', d_mode: '模式', yes: '是', no: '否',
    t_connected: '已连接到游戏', t_lost: '与游戏的连接已断开', t_owned: '已开始控制', t_released: '已释放控制', t_alloc: '已请求分配权重数组', t_installed: '桥接安装完成',
    src: ['手动', '原生动画', '原生 + 手动微调', 'RhyLive + 微调', '面捕 / 原生回落'],
    g: { eye: ['眼睛', '开合、笑眼与情绪'], gaze: ['视线', '左右上下看'], mouth: ['嘴巴', '口型与嘴角'], brow: ['眉毛', '左右眉形'], effect: ['特效', '脸红、泪、特效眼'],
         shape: ['捏脸', '脸型与五官比例'], body: ['身体', '胸部与臀部'], outfit: ['服装', '衣裙长度'], '头发': ['头发', '刘海长度'], '兽耳': ['兽耳', '长度与姿态'],
         outfit_variant: ['服装变体', '款式与细节'], nail: ['指甲', '款式与长度'], eye_detail: ['眼部细节', '瞳孔与高光'], mouth_detail: ['口部细节', '舌头与牙齿'], ear: ['兽耳', '长度与姿态'], hairstyle: ['发型', '发型款式'], hair: ['头发', '刘海与发丝'] },
  },
  en: {
    app_title: 'RE4R ShapeKey Studio', app_sub: 'Character shape keys for RE4 Remake', step_dir: 'Game folder', step_install: 'Install bridge', step_capture: 'Face capture',
    setup_title: 'Connect your game', setup_lead: 'Set your RE4 folder to prepare shape-key and face-capture controls.', dir_title: 'Game directory', dir_hint: 'Choose the folder that contains re4.exe',
    browse: 'Browse folder', validate: 'Validate', install_title: 'Install bridge', install_hint: 'Close RE4 first; existing scripts are backed up.\nAlready installed? Just start the game.',
    install_btn: 'Install / update bridge + capture', installing_btn: 'Installing…', enter: 'Enter workbench',
    dir_ok: 're4.exe found. You can enter the workbench.', dir_unsaved: 'Directory valid, but settings cannot be saved. Use a writable folder.', dir_bad: 'Choose the game folder containing re4.exe.',
    dir_none: 'No game folder set yet.', installing: 'Installing bridge; existing scripts are backed up first…', installed: 'Bridge installed. Start / restart RE4 to connect.',
    install_failed: 'Install failed. Close RE4; see installer.log.', install_nostart: 'Unable to start the installer. See the user guide.', log_unwritable: 'Cannot write installer.log. Use a writable folder.',
    dir_found: 're4.exe found', dir_wait: 'Validate a folder to unlock the workbench',
    source: 'Source', strength: 'Strength', start: 'Start control', release: 'Release', theme: 'Light / dark', topmost: '窗口置顶 / Always on top (Ctrl+T)', t_top_on: 'Always on top', t_top_off: 'Always on top off',
    connected: 'Connected to game', waiting: 'Waiting for model', offline: 'Not connected', demo_conn: 'Demo mode', conn_sub_on: 'Resident Evil 4 Remake', conn_sub_off: 'Connects once the game loads the bridge',
    nav_face: 'Expressions', nav_face_s: 'Facial expressions', nav_shape: 'Face sculpt', nav_shape_s: 'Face shape & features', nav_body: 'Body & outfit', nav_body_s: 'Figure and outfit length',
    nav_hair: 'Hair & ears', nav_hair_s: 'Bangs and animal ears', nav_preset: 'Presets', nav_preset_s: 'Save and load', nav_capture: 'RhyLive capture', nav_capture_s: 'Phone face capture',
    nav_diag: 'Diagnostics', nav_diag_s: 'Connection and meshes', nav_setup: 'Setup', nav_setup_s: 'Game folder and bridge',
    sub_face: 'Shape the facial expression live in game.', sub_shape: 'Adjust face shape and proportions.', sub_body: 'Adjust the figure and outfit length; clothes follow the body.', sub_hair: 'Adjust bangs and animal ears.',
    sub_preset: 'Save every slider as a preset or load one from a file.', sub_capture: 'Drive expressions live with RhyLive on your phone.', sub_diag: 'Connection, mesh weight arrays and runtime state.',
    nav_outfit_variant: 'Outfit variants', nav_outfit_variant_s: 'Styles and details', nav_nail: 'Nails', nav_nail_s: 'Nail style and length', nav_eye_detail: 'Eye details', nav_eye_detail_s: 'Pupil and highlight shapes',
    nav_mouth_detail: 'Mouth details', nav_mouth_detail_s: 'Tongue and teeth', nav_other: 'Other', nav_other_s: 'Uncategorised keys', nav_auto_s: 'Auto-detected section',
    sub_outfit_variant: 'Switch outfit styles and detail variants.', sub_nail: 'Adjust nail style and length.', sub_eye_detail: 'Adjust pupil and highlight shapes.', sub_mouth_detail: 'Adjust tongue and teeth.',
    sub_other: 'Keys that belong to no section.', sub_auto: 'This section is provided by the model.',
    compare: 'Neutral compare', sweep: 'Key sweep', zero_page: 'Reset page', copy: 'Copy values', paste: 'Paste values', search: 'Search shape keys…', keys: 'shape keys',
    need_control: 'Press “Start control” to adjust', no_keys: 'This model has no keys in this category', no_keys_sub: 'Body / hair keys need the model’s add-on enabled in Fluffy',
    sel_title: 'Selected parameter', sel_none: 'Click a row to see its details', p_name: 'Name', p_group: 'Group', p_manual: 'Value (local)', p_actual: 'In-game value', p_range: 'Range', p_mesh: 'Mesh', p_note: 'Notes',
    reset_one: 'Reset', locate: 'Show row', mesh_head: 'Head', mesh_body: 'Body', mesh_hair: 'Hair',
    why_alloc: 'Weight array not allocated', alloc: 'Allocate', why_unbound: 'Module not enabled',
    recv_title: 'RhyLive capture', recv_hint: 'Receive 52 ARKit channels through the game plugin', udp: 'UDP port', lan: 'Phone destination (this PC’s LAN IPv4)', lan_none: 'No LAN IPv4 found',
    lan_note: 'Use the same port on the phone. 0.0.0.0 is for PC binding only.', recv_start: 'Start / restart', recv_stop: 'Stop', recv_save: 'Save receiver settings',
    listening: 'Listening', stopped: 'Stopped', recv_wait: 'Waiting for the game and bridge', packets: 'Packets', frames: 'Face frames', age: 'Packet age',
    st_off: 'Not started', st_wait: 'Waiting for phone data', st_rx: 'Receiving', st_lost: 'Interrupted', st_lost_for: 'Interrupted {s} s', st_lost_cap: 'Interrupted >60 s',
    fps_lbl: 'Face frame rate', pps_lbl: 'Packets', total: 'Total', rejected: 'Rejected', spark_old: '60 s ago', spark_now: 'now', demo_stream: 'Demo: phone streaming',
    rt_title: 'Responsiveness', rt_smooth: 'Smooth', rt_bal: 'Balanced', rt_live: 'Realtime', rt_hint: 'Higher follows the phone more closely; lower is smoother (less jitter).', rt_na: 'Needs bridge update',
    preset_save: 'Save preset', preset_load: 'Load preset', preset_bad: 'Preset belongs to another model. Nothing loaded.', preset_ok: 'Preset loaded', copied: 'Page values copied', pasted: 'Values pasted',
    paste_bad: 'No usable values on the clipboard', nonzero: 'Non-zero keys',
    d_conn: 'Connection', d_online: 'Pipe', d_token: 'Binding', d_status: 'Panel status', d_capture: 'Capture', d_error: 'Last error', d_meshes: 'Mesh weight arrays', d_host: 'App',
    d_ready: 'Ready', d_notready: 'Not allocated', d_version: 'Version', d_dir: 'Game folder', d_mode: 'Mode', yes: 'yes', no: 'no',
    t_connected: 'Connected to the game', t_lost: 'Connection to the game lost', t_owned: 'Control started', t_released: 'Control released', t_alloc: 'Weight-array allocation requested', t_installed: 'Bridge installed',
    src: ['Manual', 'Native animation', 'Native + offsets', 'RhyLive + offsets', 'Capture / native fallback'],
    g: { eye: ['Eyes', 'Blink, smile, emotion'], gaze: ['Gaze', 'Look direction'], mouth: ['Mouth', 'Visemes and corners'], brow: ['Brows', 'Left / right brow'], effect: ['Effects', 'Blush, tears, special eyes'],
         shape: ['Sculpt', 'Face shape and features'], body: ['Body', 'Bust and hips'], outfit: ['Outfit', 'Length'], '头发': ['Hair', 'Bangs length'], '兽耳': ['Ears', 'Length and pose'],
         outfit_variant: ['Outfit variants', 'Styles and details'], nail: ['Nails', 'Style and length'], eye_detail: ['Eye details', 'Pupil and highlights'], mouth_detail: ['Mouth details', 'Tongue and teeth'], ear: ['Ears', 'Length and pose'], hairstyle: ['Hairstyle', 'Style variants'], hair: ['Hair', 'Bangs and strands'] },
  },
};
let lang = store.get('lang', 'zh');
const t = k => (T[lang][k] != null ? T[lang][k] : T.zh[k] != null ? T.zh[k] : k);

/* ------------------------------------------------------------------ pages */
/* 分区 -> 页面 映射表（唯一入口，要新增/挪动分区只改这里）。
   值是内置页面 id；表里没有的 region 会自动生成一个独立页面（页面 id = region 字符串）。
   页面标题/副标题取 T 里的 nav_<id> / nav_<id>_s / sub_<id>，没有就直接显示 region 字符串。 */
const REGION_PAGE = {
  eye: 'face', gaze: 'face', mouth: 'face', brow: 'face', effect: 'face',
  shape: 'shape',
  body: 'body', outfit: 'body',
  hair: 'hair', ear: 'hair', hairstyle: 'hair',        // 兽耳、发型并入“头发与耳朵”页
  // 其余（outfit_variant / nail / eye_detail / mouth_detail / 未知 region）自动各成一页
};
/* 内置页面：按顺序排在最前；没有任何形态键的页面会被隐藏（完全没有键时除外，保留空状态提示） */
const BUILTIN_KEY_PAGES = [
  {id: 'face', icon: 'face'}, {id: 'shape', icon: 'shape'}, {id: 'body', icon: 'body'}, {id: 'hair', icon: 'hair', byGroup: true},
];
/* 已预注册的自动页面图标；未登记的 region 使用 AUTO_ICON_DEFAULT */
const AUTO_ICON = {outfit_variant: 'hanger', nail: 'nail', eye_detail: 'pupil', mouth_detail: 'tongue'};
const AUTO_ICON_DEFAULT = 'tag';
const TAIL_PAGES = [{id: 'preset', icon: 'preset'}, {id: 'capture', icon: 'capture'}, {id: 'diag', icon: 'diag'}, {id: 'setup', icon: 'setup'}];
let PAGES = [];
/* 按当前键集合重建页面列表；返回签名，签名变了才需要重建导航 */
function rebuildPages() {
  const present = new Set(S.keys.map(k => k.region)), any = S.keys.length > 0;
  const list = [], claimed = new Set();
  for (const b of BUILTIN_KEY_PAGES) {
    const regions = Object.keys(REGION_PAGE).filter(r => REGION_PAGE[r] === b.id);
    if (any && !regions.some(r => present.has(r))) continue;
    list.push({...b, regions});
    regions.forEach(r => claimed.add(r));
  }
  for (const r of present) {            // 未被内置页面认领的 region，按出现顺序各自成页
    if (claimed.has(r) || !r) continue;
    list.push({id: r, icon: AUTO_ICON[r] || AUTO_ICON_DEFAULT, regions: [r], auto: true}); claimed.add(r);
  }
  const orphan = S.keys.filter(k => !k.region).length;      // 空 region 的键归入 face 之外的“other”页，避免丢失
  if (orphan) list.push({id: 'other', icon: AUTO_ICON_DEFAULT, regions: [''], auto: true});
  list.push({sep: true}, ...TAIL_PAGES);
  PAGES = list;
  return list.map(p => p.id || '-').join(',');
}
const pageTitle = p => (T[lang]['nav_' + p.id] || T.zh['nav_' + p.id] || (p.id === 'other' ? '' : p.id) || t('nav_other'));
const pageNavSub = p => (T[lang]['nav_' + p.id + '_s'] != null ? T[lang]['nav_' + p.id + '_s'] : T.zh['nav_' + p.id + '_s'] != null ? T.zh['nav_' + p.id + '_s'] : (p.auto ? t('nav_auto_s') : ''));
const pageSubtitle = p => (T[lang]['sub_' + p.id] != null ? T[lang]['sub_' + p.id] : T.zh['sub_' + p.id] != null ? T.zh['sub_' + p.id] : t('sub_auto'));
const GICON = {eye: 'eye', gaze: 'gaze', mouth: 'mouth', brow: 'brow', effect: 'spark', shape: 'shape', body: 'body', outfit: 'body', '头发': 'hair', '兽耳': 'ear',
  outfit_variant: 'hanger', nail: 'nail', eye_detail: 'pupil', mouth_detail: 'tongue', ear: 'ear', hairstyle: 'hair', hair: 'hair'};

/* ------------------------------------------------------------------ state */
const S = {
  view: 'setup', page: store.get('page', 'face'), search: '', online: false, token: '', owned: false, source: 1, strength: 1,
  status: '', profile: '', capture: '', error: '', compare: false, sweep: false,
  recv: {available: false, running: false, port: 9000, ip: '0.0.0.0', packets: 0, frames: 0, age: 0, rejected: 0, error: 0, status: ''},
  capSmooth: null, capHold: 0, keys: [], byId: new Map(), meshes: [], sel: store.get('sel', ''), host: {version: '0.4.2-beta.1', dir: '', valid: false, msg: 'dir_none', installing: false, lan: '', demo: DEMO},
};
const rows = new Map();           // id -> row widget on the current page
let keySig = '', pagesSig = rebuildPages();

/* ------------------------------------------------------------------ host link */
function send(...parts) { if (DEMO) return demoCommand(parts); HOST.postMessage(parts.join('\t')); }
function cmd(op, arg = '', value = '') { send('cmd', op, String(arg), String(value)); }
function hostAction(action, arg) { if (!HOST) return demoHost(action, arg); HOST.postMessage(arg == null ? `host\t${action}` : `host\t${action}\t${arg}`); }
if (HOST) HOST.addEventListener('message', e => onHost(e.data));
function onHost(m) {
  if (!m || typeof m !== 'object') return;
  if (m.t === 'snap') onSnap(m.raw || '', !!m.online);
  else if (m.t === 'host') onHostState(m);
  else if (m.t === 'preset') applyPreset(m.text || '');
}

/* ------------------------------------------------------------------ snapshot */
function onSnap(raw, online) {
  const wasOnline = S.online && !!S.token && S.token !== 'WAIT', wasOwned = S.owned, oldErr = S.error;
  S.online = online; const keys = [], meshes = []; let sawR = false;
  for (const line of raw.split('\n')) {
    const f = line.split('\t');
    if (f[0] === 'S') {
      S.token = f[1] || ''; S.owned = f[2] === '1'; S.source = +f[3] || 1; S.strength = +f[4] || 0; S.status = f[5] || ''; S.profile = f[6] || '';
      S.capture = f[7] || ''; S.error = f[8] || ''; S.compare = f[9] === '1'; S.sweep = f[10] === '1';
    } else if (f[0] === 'R') {
      S.recv = {available: f[1] === '1', running: f[2] === '1', port: +f[3] || 9000, ip: f[4] || '0.0.0.0', packets: +f[5] || 0, frames: +f[6] || 0, age: +f[7], rejected: +f[8] || 0, error: +f[9] || 0, status: f[10] || ''};
      sawR = true;
    } else if (f[0] === 'C') {                         // optional: C <cap_smooth>; absent on old bridges
      const v = parseFloat(f[1]); if (isFinite(v)) { S.capSmooth = clamp(v, 0.05, 1); }
    } else if (f[0] === 'K') {
      keys.push({id: f[1], label: f[2] || f[1], region: f[3] || '', manual: +f[4] || 0, actual: +f[5] || 0, writable: f[6] === '1', mesh: +f[7] || 1, group: f[8] || f[3] || ''});
    } else if (f[0] === 'M') {
      meshes.push({index: +f[1], name: f[2], channels: +f[3], wc: +f[4], wl: +f[5], limit: +f[6], ready: f[7] === '1'});
    }
  }
  if (!online) { S.token = ''; S.owned = false; }
  S.meshes = meshes; if (sawR) rxUpdate();
  const now = performance.now();
  for (const k of keys) {
    const old = S.byId.get(k.id);
    if (old && old.hold > now) k.manual = old.manual;       // the hand wins while dragging / typing
    k.hold = old ? old.hold : 0;
  }
  S.keys = keys; S.byId = new Map(keys.map(k => [k.id, k]));
  const sig = S.token + '|' + keys.map(k => k.id + (k.writable ? 1 : 0)).join(',');
  const connected = S.online && !!S.token && S.token !== 'WAIT';
  const first = !snapSeen; snapSeen = true;
  if (!first && connected !== wasOnline && S.view === 'bench') toast(connected ? t('t_connected') : t('t_lost'), connected ? 'ok' : 'err');
  if (!first && wasOwned !== S.owned && connected) toast(S.owned ? t('t_owned') : t('t_released'), 'ok');
  if (S.error && S.error !== oldErr) toast(S.error, 'err');
  if (sig !== keySig) { keySig = sig; const ps = rebuildPages(); if (ps !== pagesSig) { pagesSig = ps; if (!PAGES.some(p => p.id === S.page)) S.page = PAGES[0].id; if (S.view === 'bench') buildNav(); }
    if (S.view === 'bench') renderPage(false); }
  else updateRows();
  updateChrome();
}

function onHostState(h) {
  const wasInstalling = S.host.installing;
  Object.assign(S.host, h);
  if ('topmost' in h) setPin(!!h.topmost);
  if (wasInstalling && !h.installing && h.msg === 'installed') toast(t('t_installed'), 'ok');
  renderSetup();
  if (S.view === 'setup' && !booted) { booted = true; if (h.valid && Q.get('view') !== 'setup') switchView('bench', true); }
  if (S.view === 'bench' && S.page === 'diag') renderPage(false);
}
let booted = false, snapSeen = false;

/* ------------------------------------------------------------------ motion engine */
const springs = new Set();            // read-back dots and rolling numbers, one rAF loop
let rafOn = false, lastT = 0;
function tick(now) {
  const dt = Math.min(0.05, (now - (lastT || now)) / 1000); lastT = now;
  for (const s of springs) {
    if (s.kind === 'spring') {
      const k = 1 - Math.exp(-dt * (RM.matches ? 40 : 11)); s.cur += (s.target - s.cur) * k;
      if (Math.abs(s.target - s.cur) < 0.0004) { s.cur = s.target; springs.delete(s); }
      s.apply(s.cur);
    } else {
      const p = clamp((now - s.t0) / s.dur, 0, 1), e = 1 - Math.pow(1 - p, 3);
      s.apply(s.from + (s.to - s.from) * e); if (p >= 1) springs.delete(s);
    }
  }
  if (springs.size) requestAnimationFrame(tick); else { rafOn = false; lastT = 0; }
}
function kick() { if (!rafOn) { rafOn = true; requestAnimationFrame(tick); } }
function springTo(s, target) { s.target = target; s.kind = 'spring'; springs.add(s); kick(); }
function tweenNum(s, to, dur = 200) { s.kind = 'tween'; s.from = s.shown; s.to = to; s.t0 = performance.now(); s.dur = RM.matches ? 1 : dur; springs.add(s); kick(); }
function anim(node, frames, opts) { if (!node || !node.animate) return {finished: Promise.resolve()}; return node.animate(frames, Object.assign({fill: 'both'}, opts, RM.matches ? {duration: Math.min(opts.duration || 200, 110)} : {})); }
const EASE = 'cubic-bezier(.22,1,.36,1)', SPRING = 'cubic-bezier(.34,1.56,.64,1)';

/* ------------------------------------------------------------------ slider widget */
function makeSlider(host, {ticks = false, onInput, onCommit} = {}) {
  host.classList.add('slider');
  host.innerHTML = '<div class="track"></div><div class="fill"></div><div class="rb"></div><div class="thumb"></div>' + (ticks ? '<div class="ticks"><span>0.00</span><span>0.50</span><span>1.00</span></div>' : '');
  const fill = host.querySelector('.fill'), thumb = host.querySelector('.thumb'), rb = host.querySelector('.rb');
  const w = {el: host, value: 0, disabled: false, dragging: false, rbs: {cur: 0, target: 0, apply: v => { rb.style.left = (v * 100) + '%'; }}};
  w.set = v => { w.value = clamp(v, 0, 1); const p = (w.value * 100) + '%'; fill.style.width = p; thumb.style.left = p; host.classList.toggle('zero', w.value < 0.001); };
  w.readback = v => springTo(w.rbs, clamp(v, 0, 1));
  w.disable = d => { w.disabled = d; host.classList.toggle('disabled', d); };
  const at = e => { const r = host.getBoundingClientRect(); return clamp((e.clientX - r.left) / r.width, 0, 1); };
  host.addEventListener('pointerdown', e => {
    if (w.disabled || e.button !== 0) return; e.preventDefault(); host.setPointerCapture(e.pointerId); w.dragging = true; host.classList.add('drag');
    w.set(at(e)); onInput && onInput(w.value);
  });
  host.addEventListener('pointermove', e => { if (!w.dragging) return; w.set(at(e)); onInput && onInput(w.value); });
  const end = () => { if (!w.dragging) return; w.dragging = false; host.classList.remove('drag');
    anim(fill, [{filter: 'brightness(1.12)'}, {filter: 'none'}], {duration: 260, easing: EASE, fill: 'none'}); onCommit && onCommit(w.value); };
  host.addEventListener('pointerup', end); host.addEventListener('pointercancel', end); host.addEventListener('lostpointercapture', end);
  host.addEventListener('wheel', e => { if (w.disabled || !e.altKey) return; e.preventDefault(); w.set(w.value - Math.sign(e.deltaY) * 0.05); onInput && onInput(w.value); onCommit && onCommit(w.value); }, {passive: false});
  return w;
}

/* ------------------------------------------------------------------ key value plumbing */
const lastSent = new Map();
function setKey(k, v, final) {
  k.manual = v; k.hold = performance.now() + 600;
  const now = performance.now(), prev = lastSent.get(k.id) || 0;
  if (final || now - prev > 45) { lastSent.set(k.id, now); cmd('set', k.id, v.toFixed(4)); }
  const r = rows.get(k.id); if (r) { if (document.activeElement !== r.num) setNum(r, v, false); }
  if (S.sel === k.id) renderDetail();
}
function setNum(r, v, roll) { if (!roll) { r.ns.shown = v; r.num.value = fmt(v); springs.delete(r.ns); } else if (Math.abs(r.ns.shown - v) > 0.0005) tweenNum(r.ns, v); }
function canEdit(k) { return S.owned && k.writable; }
function meshOf(k) { return S.meshes.find(m => m.index === k.mesh); }
function whyLocked(k) {
  if (k.writable) return '';
  const m = meshOf(k); if (m && !m.ready) return 'alloc';
  return 'unbound';
}

/* ------------------------------------------------------------------ chrome */
function connState() { if (DEMO) return 'on'; if (!S.online) return 'off'; return S.token && S.token !== 'WAIT' ? 'on' : 'wait'; }
let lastConn = '';
function updateChrome() {
  const cs = connState();
  const conn = $('#conn'), card = $('#connCard');
  for (const n of [conn, card]) { n.classList.toggle('on', cs === 'on'); n.classList.toggle('wait', cs === 'wait'); }
  const txt = DEMO ? t('demo_conn') : cs === 'on' ? t('connected') : cs === 'wait' ? t('waiting') : t('offline');
  $('#connText').textContent = txt; $('#connCardText').textContent = txt;
  const sub = cs === 'off' ? t('conn_sub_off') : (S.status || t('conn_sub_on'));
  $('#connSub').textContent = cs === 'on' ? t('conn_sub_on') : sub; $('#connCardSub').textContent = cs === 'on' ? (S.profile || t('conn_sub_on')) : sub;
  if (cs !== lastConn) { if (cs === 'on' && lastConn) { conn.classList.remove('pulse'); void conn.offsetWidth; conn.classList.add('pulse'); } lastConn = cs; }
  $('#modelName').textContent = S.profile || (cs === 'off' ? '—' : 'Example model');
  $('#modelCount').textContent = S.keys.length ? `${S.keys.length} ${t('keys')}` : '';
  $('#portraitName').textContent = S.profile || 'Example model';
  $('#sourceText').textContent = t('src')[S.source - 1] || '—';
  const cb = $('#controlBtn'); const owned = S.owned;
  if (cb.dataset.owned !== String(owned)) {
    cb.dataset.owned = String(owned);
    cb.classList.toggle('owned', owned); cb.querySelector('use').setAttribute('href', owned ? '#i-stop' : '#i-play'); $('#controlText').textContent = owned ? t('release') : t('start');
    anim(cb, [{transform: 'scale(.96)'}, {transform: 'scale(1)'}], {duration: 320, easing: SPRING, fill: 'none'});
  }
  cb.disabled = cs !== 'on';
  if (!gain.dragging) { gain.set(clamp(S.strength / 2, 0, 1)); $('#gainText').textContent = fmt(S.strength); }
  gain.disable(!S.owned);
  $('#compareBtn').classList.toggle('on', S.compare); $('#sweepBtn').classList.toggle('on', S.sweep);
  for (const b of document.querySelectorAll('#headActions button, #subActions button')) b.disabled = !S.owned;
  const hint = $('#ownHint'); const showHint = !S.owned && isKeyPage();
  hint.innerHTML = showHint ? icon('lock') + t('need_control') : '';
  for (const it of document.querySelectorAll('.nav-item[data-page]')) { const c = it.querySelector('.count'); if (c) { const p = PAGES.find(x => x.id === it.dataset.page); c.textContent = p && p.regions ? keysOf(p, '').length || '' : ''; } }
  if (S.page === 'capture' || S.view === 'setup') updateReceiver();
}
function isKeyPage() { const p = PAGES.find(x => x.id === S.page); return !!(p && p.regions); }

/* ------------------------------------------------------------------ navigation */
function buildNav() {
  if (!PAGES.length) pagesSig = rebuildPages();
  const box = $('#navItems'), known = new Set([...box.querySelectorAll('.nav-item')].map(n => n.dataset.page)), fresh = [];
  box.innerHTML = '';
  for (const p of PAGES) {
    if (p.sep) { box.appendChild(el('div', 'nav-sep')); continue; }
    const b = el('button', 'nav-item', `${icon(p.icon)}<span class="nt"><b>${pageTitle(p)}</b><small>${pageNavSub(p)}</small></span>${p.regions ? '<span class="count"></span>' : ''}`);
    b.dataset.action = 'page'; b.dataset.page = p.id; box.appendChild(b);
    if (!known.has(p.id)) fresh.push(b);
  }
  markNav(true);
  fresh.forEach((b, i) => anim(b, [{opacity: 0, transform: 'translateX(-10px)'}, {opacity: 1, transform: 'none'}], {duration: 260, delay: 30 + i * 35, easing: EASE, fill: 'backwards'}));   // 新出现的条目依次滑入
}
function markNav(instant) {
  const cur = document.querySelector(`.nav-item[data-page="${S.page}"]`), pill = $('#navPill');
  for (const it of document.querySelectorAll('.nav-item')) it.classList.toggle('cur', it === cur);
  if (!cur) { pill.style.opacity = 0; delete pill.dataset.y; return; }
  const y = cur.offsetTop, from = pill.dataset.y != null ? +pill.dataset.y : y; pill.dataset.y = y; pill.style.opacity = 1;
  pill.style.transform = `translateY(${y}px)`; if (!instant) cur.scrollIntoView({block: 'nearest', behavior: RM.matches ? 'auto' : 'smooth'});
  if (!instant && from !== y) anim(pill, [{transform: `translateY(${from}px) scaleY(.92)`}, {transform: `translateY(${y}px) scaleY(1)`}], {duration: 420, easing: SPRING, fill: 'none'});
}
let pageAnim = 0;
async function goPage(id) {
  if (id === 'setup') { cmd('release'); switchView('setup'); return; }
  if (id === S.page) return;
  S.page = id; store.set('page', id); S.search = ''; $('#search').value = '';
  markNav(false);
  const n = ++pageAnim, page = $('#page');
  await anim(page, [{opacity: 1, transform: 'none'}, {opacity: 0, transform: 'translateY(-8px)'}], {duration: 120, easing: 'ease-in'}).finished.catch(() => {});
  if (n !== pageAnim) return;
  renderPage(true);
}

/* ------------------------------------------------------------------ pages */
function keysOf(p, q) {
  q = (q || '').trim().toLowerCase();
  return S.keys.filter(k => p.regions.includes(k.region) && (!q || k.label.toLowerCase().includes(q) || k.id.toLowerCase().includes(q) || (k.group || '').toLowerCase().includes(q)));
}
function groupsOf(p, list) {
  const order = p.byGroup ? [...new Set(list.map(k => k.group))] : p.regions;
  return order.map(g => ({id: g, keys: list.filter(k => (p.byGroup ? k.group : k.region) === g)})).filter(g => g.keys.length);
}
function renderPage(enter) {
  const p = PAGES.find(x => x.id === S.page) || PAGES[0];
  $('#pageIcon').innerHTML = icon(p.icon); $('#pageTitle').textContent = pageTitle(p); $('#pageSub').textContent = pageSubtitle(p);
  $('#headActions').classList.toggle('hidden', !p.regions); $('.subhead').classList.toggle('hidden', !p.regions);
  const page = $('#page'); page.innerHTML = ''; rows.clear();
  if (p.regions) renderKeys(p, page);
  else if (p.id === 'preset') renderPresets(page);
  else if (p.id === 'capture') { const c = el('article', 'card receiver-card capture-card'); page.appendChild(c); renderReceiver(c); }
  else if (p.id === 'diag') renderDiag(page);
  updateChrome(); renderDetail();
  page.getAnimations().forEach(a => a.cancel());
  if (enter) {
    anim(page, [{opacity: 0, transform: 'translateY(16px)'}, {opacity: 1, transform: 'none'}], {duration: 240, easing: EASE, fill: 'none'});
    const cards = page.querySelectorAll('.group, .card');
    cards.forEach((c, i) => anim(c, [{opacity: 0, transform: 'translateY(14px)'}, {opacity: 1, transform: 'none'}], {duration: 260, delay: 40 + i * 40, easing: EASE, fill: 'backwards'}));
    const first = page.querySelectorAll('.krow');
    Array.from(first).slice(0, 14).forEach((r, i) => anim(r, [{opacity: 0, transform: 'translateY(8px)'}, {opacity: 1, transform: 'none'}], {duration: 220, delay: 60 + i * 18, easing: EASE, fill: 'backwards'}));
  }
}
function renderKeys(p, page) {
  const list = keysOf(p, S.search);
  $('#pageBadge').textContent = `${list.length} ${t('keys')}`;
  if (!list.length) {
    page.appendChild(el('div', 'empty-state', `${icon(p.icon)}<div><b>${t('no_keys')}</b><p class="muted">${S.search ? '' : t('no_keys_sub')}</p></div>`)); return;
  }
  const grid = el('div', 'groups'); page.appendChild(grid);
  const closed = JSON.parse(store.get('closed', '{}'));
  for (const g of groupsOf(p, list)) {
    const [gt, gs] = (T[lang].g[g.id] || T.zh.g[g.id] || [g.id, '']);
    const card = el('section', 'group' + (closed[g.id] ? ' closed' : ''));
    const head = el('button', 'group-head', `<span class="gicon">${icon(GICON[g.id] || 'spark')}</span><span class="gt"><b>${gt}</b><small class="muted">${gs}</small></span><span class="gcount">${g.keys.length}</span><svg class="chev" viewBox="0 0 24 24"><use href="#i-chevron"/></svg>`);
    const body = el('div', 'group-body'), inner = el('div', 'group-inner'); body.appendChild(inner);
    if (closed[g.id]) body.style.height = '0px';
    head.addEventListener('click', () => toggleGroup(card, body, g.id));
    card.append(head, body); grid.appendChild(card);
    for (const k of g.keys) inner.appendChild(makeRow(k));
  }
}
function toggleGroup(card, body, id) {
  const closed = JSON.parse(store.get('closed', '{}')); const close = !card.classList.contains('closed');
  const h = body.scrollHeight; card.classList.toggle('closed', close);
  body.style.height = (close ? h : 0) + 'px'; void body.offsetHeight; body.style.height = (close ? 0 : h) + 'px';
  if (!close) body.addEventListener('transitionend', function f() { body.style.height = ''; body.removeEventListener('transitionend', f); });
  if (close) closed[id] = 1; else delete closed[id]; store.set('closed', JSON.stringify(closed));
}
function makeRow(k) {
  const r = el('div', 'krow'); r.dataset.id = k.id;
  const lab = el('div', 'klabel'); lab.textContent = k.label;
  const sh = el('div'); const num = el('input', 'numbox'); num.inputMode = 'decimal';
  const reset = el('button', 'reset-one', icon('reset')); reset.title = t('reset_one');
  r.append(lab, sh, num, reset);
  const row = {el: r, k, num, reset, ns: {shown: k.manual, apply: v => { row.ns.shown = v; num.value = fmt(v); }}};
  row.slider = makeSlider(sh, {ticks: true, onInput: v => { selectKey(k.id); setKey(S.byId.get(k.id) || k, v, false); }, onCommit: v => setKey(S.byId.get(k.id) || k, v, true)});
  row.slider.set(k.manual); row.slider.rbs.cur = k.actual; row.slider.rbs.apply(k.actual); num.value = fmt(k.manual);
  num.addEventListener('focus', () => { selectKey(k.id); num.select(); });
  num.addEventListener('keydown', e => { if (e.key === 'Enter') num.blur(); if (e.key === 'Escape') { num.value = fmt((S.byId.get(k.id) || k).manual); num.blur(); }
    if (e.key === 'ArrowUp' || e.key === 'ArrowDown') { e.preventDefault(); const kk = S.byId.get(k.id) || k; const v = clamp(kk.manual + (e.key === 'ArrowUp' ? .01 : -.01) * (e.shiftKey ? 10 : 1), 0, 1); row.slider.set(v); setKey(kk, v, true); num.value = fmt(v); } });
  num.addEventListener('change', () => { const kk = S.byId.get(k.id) || k; const v = parseFloat(num.value); if (isFinite(v) && canEdit(kk)) { row.slider.set(clamp(v, 0, 1)); setKey(kk, clamp(v, 0, 1), true); } num.value = fmt(kk.manual); });
  reset.addEventListener('click', e => { e.stopPropagation(); const kk = S.byId.get(k.id) || k; animateTo(row, kk, 0); });
  r.addEventListener('pointerdown', () => selectKey(k.id));
  rows.set(k.id, row); applyRow(row, k, false);
  return r;
}
function animateTo(row, k, v) {             // reset-to-zero glides the thumb instead of jumping
  const from = k.manual, s = {shown: from, apply: x => { row.slider.set(x); row.num.value = fmt(x); }};
  tweenNum(s, v, 260); setKey(k, v, true);
}
function applyRow(row, k, roll) {
  const edit = canEdit(k), why = whyLocked(k);
  row.el.classList.toggle('off', !edit); row.slider.disable(!edit); row.num.disabled = !edit; row.reset.disabled = !edit;
  row.el.classList.toggle('sel', S.sel === k.id);
  let tag = row.el.querySelector('.why');
  if (why) {
    const lab = row.el.querySelector('.klabel');
    if (!tag) { tag = el('small', 'why'); lab.appendChild(tag); }
    tag.innerHTML = why === 'alloc' ? `${t('why_alloc')}` : t('why_unbound');
    row.el.title = why === 'alloc' ? t('why_alloc') : t('why_unbound');
  } else if (tag) { tag.remove(); row.el.title = ''; }
  if (!row.slider.dragging && !(k.hold > performance.now())) { row.slider.set(k.manual); if (document.activeElement !== row.num) setNum(row, k.manual, roll); }
  row.slider.readback(k.actual);
}
function updateRows() { for (const [id, row] of rows) { const k = S.byId.get(id); if (k) applyRow(row, k, true); } if (S.page === 'diag') renderDiagLive(); renderDetail(); }
function selectKey(id) {
  if (S.sel === id) return; const old = rows.get(S.sel); if (old) old.el.classList.remove('sel');
  S.sel = id; store.set('sel', id); const r = rows.get(id); if (r) r.el.classList.add('sel'); renderDetail(true);
}

/* ------------------------------------------------------------------ inspector */
let detailId = null;
function renderDetail(changed) {
  const d = $('#detail'), k = S.byId.get(S.sel);
  if (!k) { d.innerHTML = `<h4>${icon('preset')}${t('sel_title')}</h4><div class="empty">${t('sel_none')}</div>`; detailId = null; return; }
  const meshName = [, t('mesh_head'), t('mesh_body'), t('mesh_hair')][k.mesh] || ('#' + k.mesh);
  const [gt] = T[lang].g[k.region] || T[lang].g[k.group] || [k.group];
  if (detailId !== k.id) {
    d.innerHTML = `<h4>${icon('preset')}${t('sel_title')}</h4>
      <div class="line"><span>${t('p_name')}</span><span class="v-name"></span></div>
      <div class="line"><span>${t('p_group')}</span><span class="v-group"></span></div>
      <div class="line"><span>${t('p_mesh')}</span><span class="v-mesh"></span></div>
      <div class="line"><span>${t('p_manual')}</span><span class="readout num v-manual"></span></div>
      <div class="line"><span>${t('p_actual')}</span><span class="readout num rbv v-actual"></span></div>
      <div class="line"><span>${t('p_range')}</span><span class="num">0.00 ~ 1.00</span></div>
      <div class="cap">${t('p_note')}</div><div class="note v-note"></div>
      <div class="btns"><button class="secondary" data-action="reset-sel">${icon('reset')}${t('reset_one')}</button><button class="primary" data-action="locate">${icon('search')}${t('locate')}</button></div>`;
    detailId = k.id;
    if (changed) anim(d, [{opacity: .4, transform: 'translateY(4px)'}, {opacity: 1, transform: 'none'}], {duration: 200, easing: EASE, fill: 'none'});
  }
  d.querySelector('.v-name').textContent = k.label; d.querySelector('.v-group').textContent = gt || k.group; d.querySelector('.v-mesh').textContent = meshName;
  d.querySelector('.v-manual').textContent = fmt(k.manual); d.querySelector('.v-actual').textContent = fmt(k.actual);
  const why = whyLocked(k);
  d.querySelector('.v-note').textContent = `${k.id}${why ? ' · ' + (why === 'alloc' ? t('why_alloc') : t('why_unbound')) : ''}`;
  const rs = d.querySelector('[data-action="reset-sel"]'); if (rs) rs.disabled = !canEdit(k);
}

/* ------------------------------------------------------------------ presets / diagnostics / receiver */
function presetText() { return `RE4R_SHAPEKEY_PRESET_V1\t${S.profile}\n` + S.keys.map(k => `${k.id}\t${k.manual.toFixed(6)}`).join('\n') + '\n'; }
function applyPreset(text, silentPaste) {
  const lines = text.replace(/\r/g, '').split('\n'); const head = lines[0].split('\t');
  if (head[0] !== 'RE4R_SHAPEKEY_PRESET_V1' || head[1] !== S.profile) { toast(silentPaste ? t('paste_bad') : t('preset_bad'), 'err'); return; }
  if (!silentPaste) cmd('zero');
  let n = 0;
  for (const l of lines.slice(1)) { const [id, v] = l.split('\t'); const k = S.byId.get(id), x = parseFloat(v); if (k && isFinite(x) && x >= 0 && x <= 1 && canEdit(k)) { const row = rows.get(id); if (row) animateTo(row, k, x); else setKey(k, x, true); n++; } }
  toast(silentPaste ? `${t('pasted')} · ${n}` : `${t('preset_ok')} · ${n}`, 'ok');
}
function renderPresets(page) {
  const c = el('article', 'card');
  c.innerHTML = `<h2>${t('nav_preset')}</h2><p class="muted">${t('sub_preset')}</p><div class="row2" style="margin-top:16px;max-width:520px">
    <button class="primary" data-action="preset-save">${icon('save')}${t('preset_save')}</button><button class="secondary" data-action="preset-load">${icon('folder')}${t('preset_load')}</button></div>
    <h2 style="margin-top:24px">${t('nonzero')}</h2><div class="kvlist" id="nonzero"></div>`;
  page.appendChild(c);
  const nz = c.querySelector('#nonzero'); const list = S.keys.filter(k => k.manual > 0.0005);
  nz.innerHTML = list.length ? list.map(k => `<span>${esc(k.label)}</span><span class="num">${fmt(k.manual)}</span>`).join('') : `<span>—</span><span></span>`;
}
const esc = s => String(s).replace(/[&<>"]/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}[c]));
function renderDiag(page) {
  const g = el('div', 'diag-grid'); page.appendChild(g);
  g.innerHTML = `<article class="card"><h2>${t('d_conn')}</h2><div class="kvlist" id="dConn"></div></article>
    <article class="card"><h2>${t('d_meshes')}</h2><div id="dMesh"></div></article>
    <article class="card"><h2>${t('d_host')}</h2><div class="kvlist" id="dHost"></div></article>`;
  renderDiagLive();
}
function renderDiagLive() {
  const c = $('#dConn'); if (!c) return;
  c.innerHTML = [[t('d_online'), S.online || DEMO ? t('yes') : t('no')], [t('d_token'), S.token || '—'], [t('d_status'), S.status || '—'], [t('d_capture'), S.capture || '—'], [t('d_error'), S.error || '—']]
    .map(([a, b]) => `<span>${a}</span><span>${esc(b)}</span>`).join('');
  $('#dMesh').innerHTML = S.meshes.length ? S.meshes.map(m => `<div class="mesh-row"><div><b>${m.index}. ${esc(m.name)}</b><small>${m.channels} ch · WeightCount ${m.wc} · List ${m.wl} · limit ${m.limit}</small></div>
      ${m.ready ? `<span class="pill ok">${t('d_ready')}</span>` : `<button class="sm primary" data-action="alloc" data-mesh="${m.index}">${icon('bolt')}${t('alloc')}</button>`}</div>`).join('') : '<p class="muted">—</p>';
  $('#dHost').innerHTML = [[t('d_version'), S.host.version], [t('d_dir'), S.host.dir || '—'], [t('d_mode'), DEMO ? 'demo' : 'live']].map(([a, b]) => `<span>${a}</span><span>${esc(b)}</span>`).join('');
}
function renderReceiver(c) {
  c.innerHTML = `<h2>${t('recv_title')}</h2><p class="muted">${t('recv_hint')}</p>
    <div class="kv"><label>${t('udp')}</label><input class="input recv-port" value="${S.recv.port || 9000}"></div>
    <div class="kv"><label>Bind IPv4</label><input class="input recv-ip" value="${esc(S.recv.ip || '0.0.0.0')}"></div>
    <div class="lan"><small class="muted">${t('lan')}</small><br><b class="num recv-lan"></b><br><small class="muted">${t('lan_note')}</small></div>
    <div class="recv-state"><span class="dot"></span><span class="recv-text"></span></div>
    <div class="stream">
      <div class="stream-head"><span class="chip st-chip"></span>${DEMO ? `<button class="sm secondary demo-stream" data-action="demo-stream">${t('demo_stream')}</button>` : ''}</div>
      <div class="stats">
        <div class="stat"><small>${t('fps_lbl')}</small><b class="num"><span class="st-fps">0</span><i> fps</i></b><em class="st-ftot"></em></div>
        <div class="stat"><small>${t('pps_lbl')}</small><b class="num"><span class="st-pps">0</span><i> /s</i></b><em class="st-ptot"></em></div>
        <div class="stat"><small>${t('age')}</small><b class="num st-a">—</b><em class="st-rej"></em></div>
      </div>
      <canvas class="spark" aria-hidden="true"></canvas>
      <div class="spark-axis"><span>${t('spark_old')}</span><span>${t('spark_now')}</span></div>
      <div class="rt">
        <div class="rt-head"><b>${t('rt_title')}</b><span class="num rt-val">—</span></div>
        <div class="rt-body"><div class="rt-slider"></div></div>
        <div class="rt-btns"><button class="sm secondary" data-action="rt" data-v="0.25">${t('rt_smooth')}</button><button class="sm secondary" data-action="rt" data-v="0.5">${t('rt_bal')}</button><button class="sm secondary" data-action="rt" data-v="1">${t('rt_live')}</button></div>
        <p class="muted rt-hint"></p>
      </div>
    </div>
    <div class="row2"><button class="primary" data-action="recv" data-op="receiver_start">${icon('play')}${t('recv_start')}</button><button class="secondary" data-action="recv" data-op="receiver_stop">${icon('stop')}${t('recv_stop')}</button></div>
    <button class="secondary wide" data-action="recv" data-op="receiver_config">${icon('save')}${t('recv_save')}</button>
    <p class="muted recv-status" style="margin-top:12px;font-size:12.5px"></p>`;
  const rs = c.querySelector('.rt-slider');
  c._rt = makeSlider(rs, {onInput: v => { const x = fromRt(v); S.capSmooth = x; S.capHold = performance.now() + 800; showRt(); throttleRt(x); }, onCommit: v => { sendRt(fromRt(v)); }});
  updateReceiver();
}
/* ---- stream rates / state / sparkline (rates computed here from the cumulative counters) ---- */
const RX = {win: [], fps: 0, pps: 0, lastF: -1, lastP: -1, lastFrameT: 0, hist: [], tickT: 0};
const HN = 240;                                       // 60 s at 4 Hz
const toRt = x => clamp((x - 0.05) / 0.95, 0, 1), fromRt = v => Math.round((0.05 + v * 0.95) * 100) / 100;
function sendRt(x) { cmd('cap_smooth', '', x.toFixed(2)); }
let rtT = 0; function throttleRt(x) { const n = performance.now(); if (n - rtT > 80) { rtT = n; sendRt(x); } }
function showRt() {
  const has = S.capSmooth != null, x = has ? S.capSmooth : 0.5;
  for (const c of document.querySelectorAll('.receiver-card')) {
    if (!c._rt) continue;
    if (!c._rt.dragging) c._rt.set(toRt(x));
    c._rt.disable(!has);
    c.querySelector('.rt').classList.toggle('na', !has);
    c.querySelector('.rt-val').textContent = has ? x.toFixed(2) : '—';
    c.querySelector('.rt-hint').textContent = has ? t('rt_hint') : t('rt_na') + ' · ' + t('rt_hint');
    for (const b of c.querySelectorAll('[data-action="rt"]')) { b.disabled = !has; b.classList.toggle('on', has && Math.abs(+b.dataset.v - x) < 0.005); }
  }
}
function rxUpdate() {                                 // call once per snapshot
  const r = S.recv, now = performance.now();
  if (RX.lastF >= 0 && (r.frames < RX.lastF || r.packets < RX.lastP)) { RX.win = []; RX.fps = RX.pps = 0; }       // counter reset (receiver restarted)
  if (r.frames > RX.lastF && RX.lastF >= 0) RX.lastFrameT = now;
  if (RX.lastF < 0) RX.lastFrameT = now;
  RX.lastF = r.frames; RX.lastP = r.packets;
  RX.win.push({t: now, f: r.frames, p: r.packets});
  while (RX.win.length > 2 && now - RX.win[1].t >= 1000) RX.win.shift();
  const a = RX.win[0], b = RX.win[RX.win.length - 1], dt = (b.t - a.t) / 1000;
  if (dt >= 0.3) { RX.fps = Math.max(0, (b.f - a.f) / dt); RX.pps = Math.max(0, (b.p - a.p) / dt); }
}
function streamState() {
  const r = S.recv, ok = DEMO || (S.online && r.available);
  if (!ok || !r.running) return 'off';
  if (!(r.frames > 0)) return 'wait';
  return r.age >= 0 && r.age <= 500 ? 'rx' : 'lost';
}
function lostText() {
  const a = S.recv.age, s = a >= 0 && a < 1e9 ? a / 1000 : (performance.now() - RX.lastFrameT) / 1000;
  return s > 60 ? t('st_lost_cap') : t('st_lost_for').replace('{s}', Math.max(1, Math.floor(s)));
}
const nf = v => Math.round(v).toLocaleString('en-US');
function rtState() { return S.capSmooth != null; }
function sparkSample() {
  const st = streamState(); RX.hist.push(st === 'rx' ? RX.fps : 0); if (RX.hist.length > HN) RX.hist.shift(); RX.tickT = performance.now();
  if (RM.matches) drawSparks(0);
}
function drawSparks(frac) {
  const cvs = document.querySelectorAll('canvas.spark'); if (!cvs.length) return;
  const cs = getComputedStyle(document.documentElement), rx = streamState() === 'rx';
  const col = (rx ? cs.getPropertyValue('--green') : cs.getPropertyValue('--muted')).trim() || '#888', grid = cs.getPropertyValue('--line').trim() || '#ccc';
  const h = RX.hist, n = h.length, mx = Math.max(30, ...h) * 1.15;
  for (const cv of cvs) {
    const w = cv.clientWidth, hh = cv.clientHeight; if (!w || !hh) continue;
    const dpr = window.devicePixelRatio || 1; if (cv.width !== Math.round(w * dpr) || cv.height !== Math.round(hh * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(hh * dpr); }
    const g = cv.getContext('2d'); g.setTransform(dpr, 0, 0, dpr, 0, 0); g.clearRect(0, 0, w, hh);
    g.strokeStyle = grid; g.lineWidth = 1; g.beginPath(); for (const y of [0.25, 0.5, 0.75]) { g.moveTo(0, Math.round(hh * y) + .5); g.lineTo(w, Math.round(hh * y) + .5); } g.stroke();
    if (n < 2) continue;
    const step = w / (HN - 1), X = i => w - (n - 1 - i + frac) * step, Y = v => hh - 3 - (v / mx) * (hh - 8);
    g.beginPath(); g.moveTo(X(0), Y(h[0])); for (let i = 1; i < n; i++) { const x0 = X(i - 1), x1 = X(i), xm = (x0 + x1) / 2; g.bezierCurveTo(xm, Y(h[i - 1]), xm, Y(h[i]), x1, Y(h[i])); }
    g.strokeStyle = col; g.lineWidth = 1.8; g.lineJoin = 'round'; g.stroke();
    g.lineTo(X(n - 1), hh); g.lineTo(X(0), hh); g.closePath(); g.globalAlpha = rx ? .16 : .08; g.fillStyle = col; g.fill(); g.globalAlpha = 1;
  }
}
function sparkLoop() {
  if (!RM.matches && !document.hidden) drawSparks(clamp((performance.now() - RX.tickT) / 250, 0, 1));
  requestAnimationFrame(sparkLoop);
}
setInterval(sparkSample, 250); requestAnimationFrame(sparkLoop);
function updateReceiver() {
  for (const c of document.querySelectorAll('.receiver-card')) {
    if (!c.querySelector('.recv-lan')) continue;
    const ok = DEMO || (S.online && S.recv.available);
    c.querySelector('.recv-lan').textContent = S.host.lan || t('lan_none');
    const st = c.querySelector('.recv-state'); st.classList.toggle('on', ok && S.recv.running);
    c.querySelector('.recv-text').textContent = ok ? (S.recv.running ? t('listening') : t('stopped')) : t('recv_wait');
    const ss = ok ? streamState() : 'off', chip = c.querySelector('.st-chip');
    chip.className = 'chip st-chip ' + ss;
    chip.textContent = ss === 'rx' ? t('st_rx') : ss === 'lost' ? lostText() : ss === 'wait' ? t('st_wait') : t('st_off');
    const fpsN = ss === 'rx' ? RX.fps : 0, ppsN = ss === 'rx' ? RX.pps : 0;
    c.querySelector('.stream').dataset.state = ss;
    c.querySelector('.st-fps').textContent = fpsN.toFixed(fpsN < 10 && fpsN > 0 ? 1 : 0); c.querySelector('.st-pps').textContent = ppsN.toFixed(0);
    c.querySelector('.st-ftot').textContent = `${t('total')} ${nf(S.recv.frames)}`; c.querySelector('.st-ptot').textContent = `${t('total')} ${nf(S.recv.packets)}`;
    c.querySelector('.st-a').textContent = ss === 'rx' ? S.recv.age + ' ms' : '—';
    c.querySelector('.st-rej').textContent = S.recv.rejected > 0 ? `${t('rejected')} ${nf(S.recv.rejected)}` : '';
    const dm = c.querySelector('.demo-stream'); if (dm) dm.classList.toggle('on', !!(demoState && demoState.recv.stream));
    const rtbox = c.querySelector('.rt');
    if (c._rt && rtbox && !c._rt.dragging && !(S.capHold > performance.now())) showRt();
    c.querySelector('.recv-status').textContent = S.recv.status || '';
    for (const b of c.querySelectorAll('[data-action="recv"]')) b.disabled = !ok;
  }
}

/* ------------------------------------------------------------------ setup view */
function renderSetup() {
  const h = S.host;
  if (document.activeElement !== $('#dirInput')) $('#dirInput').value = h.dir || '';
  const box = $('#dirState'); box.classList.toggle('ok', !!h.valid); $('#dirStateText').textContent = h.valid ? t('dir_found') : t('dir_wait');
  $('#setupMsg').textContent = t(h.msg || 'dir_none');
  const ib = $('#installBtn'); ib.classList.toggle('busy', !!h.installing); ib.disabled = !h.valid || h.installing || DEMO;
  $('#installText').textContent = h.installing ? t('installing_btn') : t('install_btn');
  $('#enterBtn').disabled = !h.valid || h.installing;
  $('#setupVersion').textContent = `Setup · ${h.version}${DEMO ? ' · demo' : ''}`;
  $('#verText').textContent = `RE4R ShapeKey Studio\nv${h.version}`;
  const step = !h.valid ? 1 : h.installing ? 2 : 3;
  document.querySelectorAll('.step').forEach(s => { const n = +s.dataset.step; s.classList.toggle('cur', n === step); s.classList.toggle('done', n < step); });
  const rc = $('#receiverCardSetup'); if (!rc.querySelector('.recv-lan')) renderReceiver(rc); else updateReceiver();
}
async function switchView(v, instant) {
  if (S.view === v) return;
  const from = $('#' + S.view), to = $('#' + v); S.view = v;
  if (!instant) await anim(from, [{opacity: 1, transform: 'none'}, {opacity: 0, transform: 'scale(.985)'}], {duration: 140, easing: 'ease-in'}).finished.catch(() => {});
  from.classList.add('hidden'); from.getAnimations().forEach(a => a.cancel()); to.classList.remove('hidden');
  if (v === 'bench') { buildNav(); if (!PAGES.find(p => p.id === S.page && p.id !== 'setup')) S.page = 'face'; markNav(true); renderPage(true); }
  else renderSetup();
  anim(to, [{opacity: 0, transform: 'scale(1.012)'}, {opacity: 1, transform: 'none'}], {duration: instant ? 1 : 280, easing: EASE, fill: 'none'});
}

/* ------------------------------------------------------------------ toasts */
function toast(msg, kind) {
  const box = $('#toasts'); const n = el('div', 'toast ' + (kind || ''), `<span class="bar"></span><span></span>`); n.lastChild.textContent = msg; box.appendChild(n);
  while (box.children.length > 3) box.firstChild.remove();
  anim(n, [{opacity: 0, transform: 'translateY(14px) scale(.98)'}, {opacity: 1, transform: 'none'}], {duration: 260, easing: EASE, fill: 'none'});
  setTimeout(() => anim(n, [{opacity: 1, transform: 'none'}, {opacity: 0, transform: 'translateX(24px)'}], {duration: 220, easing: 'ease-in'}).finished.then(() => n.remove()), 3200);
}

/* ------------------------------------------------------------------ theme / language */
let theme = Q.get('theme') || store.get('theme', matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
function applyTheme() { document.documentElement.dataset.theme = theme; hostAction('theme', theme); }
let pinned = false;
function setPin(on) { pinned = on; const b = $('#pinBtn'); if (b) { b.classList.toggle('on', on); b.setAttribute('aria-pressed', on ? 'true' : 'false'); } }
function toggleTopmost() { const on = !pinned; setPin(on); hostAction('topmost', on ? '1' : '0'); toast(t(on ? 't_top_on' : 't_top_off'), 'ok'); }
function toggleTheme(btn) {
  const next = theme === 'dark' ? 'light' : 'dark';
  const go = () => { theme = next; store.set('theme', theme); applyTheme(); };
  if (!document.startViewTransition || RM.matches) return go();
  const r = btn.getBoundingClientRect(), x = r.left + r.width / 2, y = r.top + r.height / 2, R = Math.hypot(Math.max(x, innerWidth - x), Math.max(y, innerHeight - y));
  document.startViewTransition(go).ready.then(() => document.documentElement.animate({clipPath: [`circle(0 at ${x}px ${y}px)`, `circle(${R}px at ${x}px ${y}px)`]}, {duration: 520, easing: 'cubic-bezier(.65,0,.35,1)', pseudoElement: '::view-transition-new(root)'}));
}
function applyLang() {
  document.documentElement.lang = lang === 'zh' ? 'zh-CN' : 'en';
  document.querySelectorAll('[data-i18n]').forEach(n => { n.textContent = t(n.dataset.i18n); });
  document.querySelectorAll('[data-i18n-title]').forEach(n => { n.title = t(n.dataset.i18nTitle); });
  $('#search').placeholder = t('search'); $('#langText').textContent = lang === 'zh' ? '简体中文' : 'English';
  $('#sourcePop').innerHTML = t('src').map((s, i) => `<button data-action="source" data-src="${i + 1}">${s}</button>`).join('');
  $('#controlBtn').dataset.owned = '';
  if (S.view === 'bench') { buildNav(); renderPage(false); } renderSetup(); renderReceiver($('#receiverCardSetup')); updateChrome();
}

/* ------------------------------------------------------------------ events */
const gain = makeSlider($('#gainSlider'), {onInput: v => { $('#gainText').textContent = fmt(v * 2); throttleGain(v * 2); }, onCommit: v => cmd('gain', (v * 2).toFixed(3))});
let gainT = 0; function throttleGain(v) { const n = performance.now(); if (n - gainT > 60) { gainT = n; cmd('gain', v.toFixed(3)); } }
document.addEventListener('click', e => {
  const b = e.target.closest('[data-action]'); if (!b || b.disabled) { closeSelect(e); return; }
  const a = b.dataset.action;
  if (a !== 'source-open') closeSelect(e);
  switch (a) {
    case 'page': if (S.view === 'bench') goPage(b.dataset.page); break;
    case 'control': cmd(S.owned ? 'release' : 'start'); break;
    case 'source-open': $('#sourceSelect').classList.toggle('open'); markSource(); break;
    case 'source': cmd('source', b.dataset.src); S.source = +b.dataset.src; updateChrome(); break;
    case 'compare': cmd('compare', S.compare ? '0' : '1'); break;
    case 'sweep': cmd('sweep', S.sweep ? '0' : '1'); break;
    case 'zero-page': { const p = PAGES.find(x => x.id === S.page); if (p && p.regions) for (const k of keysOf(p, '')) if (canEdit(k) && k.manual > 0) { const r = rows.get(k.id); r ? animateTo(r, k, 0) : setKey(k, 0, true); } break; }
    case 'copy': { const p = PAGES.find(x => x.id === S.page); const ks = p && p.regions ? keysOf(p, '') : S.keys;
      const txt = `RE4R_SHAPEKEY_PRESET_V1\t${S.profile}\n` + ks.map(k => `${k.id}\t${k.manual.toFixed(6)}`).join('\n') + '\n';
      (navigator.clipboard ? navigator.clipboard.writeText(txt) : Promise.reject()).then(() => toast(t('copied'), 'ok'), () => toast(t('paste_bad'), 'err')); break; }
    case 'paste': (navigator.clipboard ? navigator.clipboard.readText() : Promise.reject()).then(x => applyPreset(x, true), () => toast(t('paste_bad'), 'err')); break;
    case 'preset-save': hostAction('preset_save', presetText()); break;
    case 'preset-load': hostAction('preset_load'); break;
    case 'reset-sel': { const k = S.byId.get(S.sel); const r = rows.get(S.sel); if (k) r ? animateTo(r, k, 0) : setKey(k, 0, true); break; }
    case 'locate': { const r = rows.get(S.sel); if (r) { r.el.scrollIntoView({behavior: RM.matches ? 'auto' : 'smooth', block: 'center'}); anim(r.el, [{boxShadow: '0 0 0 2px var(--accent)'}, {boxShadow: '0 0 0 0 transparent'}], {duration: 900, easing: 'ease-out', fill: 'none'}); }
      else { const k = S.byId.get(S.sel); const p = k && PAGES.find(x => x.regions && x.regions.includes(k.region)); if (p) goPage(p.id).then(() => setTimeout(() => { const rr = rows.get(S.sel); rr && rr.el.scrollIntoView({block: 'center'}); }, 300)); } break; }
    case 'alloc': cmd('alloc', b.dataset.mesh || ''); toast(t('t_alloc'), 'ok'); break;
    case 'recv': { const c = b.closest('.receiver-card'); const port = c.querySelector('.recv-port').value.trim(), ip = c.querySelector('.recv-ip').value.trim();
      if (b.dataset.op !== 'receiver_stop' && (!(+port >= 1 && +port <= 65535) || !/^\d{1,3}(\.\d{1,3}){3}$/.test(ip))) { toast(lang === 'zh' ? 'UDP 端口必须为 1–65535，地址必须为 IPv4。' : 'Port must be 1–65535 and the address IPv4.', 'err'); break; }
      cmd(b.dataset.op, port, ip); break; }
    case 'rt': { const x = +b.dataset.v; S.capSmooth = x; S.capHold = performance.now() + 800; showRt(); sendRt(x); break; }
    case 'demo-stream': if (demoState) { demoState.recv.stream = !demoState.recv.stream; if (!demoState.recv.stream) demoState.recv.stopT = performance.now(); } break;
    case 'browse': hostAction('browse'); break;
    case 'validate': hostAction('validate', $('#dirInput').value.trim()); break;
    case 'install': hostAction('install'); break;
    case 'enter': if (S.host.valid) switchView('bench'); break;
    case 'theme': toggleTheme(b); break;
    case 'topmost': toggleTopmost(); break;
    case 'lang': lang = lang === 'zh' ? 'en' : 'zh'; store.set('lang', lang); applyLang(); break;
  }
});
function markSource() { $('#sourcePop').querySelectorAll('button').forEach(b => b.classList.toggle('cur', +b.dataset.src === S.source)); }
function closeSelect(e) { if (!e || !e.target.closest('#sourceSelect')) $('#sourceSelect').classList.remove('open'); }
$('#search').addEventListener('input', e => { S.search = e.target.value; renderPage(false); });
$('#dirInput').addEventListener('keydown', e => { if (e.key === 'Enter') hostAction('validate', e.target.value.trim()); });
document.addEventListener('keydown', e => {
  if (e.ctrlKey && e.key.toLowerCase() === 'f' && S.view === 'bench') { e.preventDefault(); $('#search').focus(); }
  if (e.ctrlKey && !e.shiftKey && !e.altKey && e.key.toLowerCase() === 't') { e.preventDefault(); toggleTopmost(); }
  if (e.key === 'Escape') closeSelect();
});
addEventListener('resize', () => markNav(true));

/* ------------------------------------------------------------------ demo engine (plain browser / --demo) */
let demoState = null, demoTimer = 0;
/* 仅演示模式：假的新分区键，用来离线查看自动生成的分区页（?fake=0 关闭）。真实数据不会走到这里 */
const DEMO_FAKE_KEYS = [
  ['nail', '指甲', 'nail_long', '指甲长度'], ['nail', '指甲', 'nail_pointed', '尖指甲'], ['nail', '指甲', 'nail_round', '圆指甲'],
  ['outfit_variant', '服装变体', 'ov_maid', '女仆款'], ['outfit_variant', '服装变体', 'ov_casual', '便装款'], ['outfit_variant', '服装变体', 'ov_ruffle', '荷叶边'], ['outfit_variant', '服装变体', 'ov_ribbon', '蝴蝶结'],
  ['eye_detail', '眼部细节', 'ed_pupil_star', '星形瞳孔'], ['eye_detail', '眼部细节', 'ed_pupil_slit', '竖瞳'], ['eye_detail', '眼部细节', 'ed_highlight_big', '大高光'],
].map(([region, group, id, label]) => ({id: 'DEMO_' + id, label, region, group, manual: 0, writable: true, mesh: 1}));
function demoInit() {
  const keys = (window.DEMO_KEYS || []).concat(Q.get('fake') === '0' ? [] : DEMO_FAKE_KEYS).map(k => ({...k, manual: k.manual || 0, actual: k.manual || 0}));
  demoState = {owned: Q.get('owned') === '1', source: 1, strength: 1, compare: 0, sweep: 0, keys, cap: 0.5, recv: {running: true, stream: Q.get('stream') !== '0', pacc: 0, facc: 0, packets: 1840, frames: 1792, lastT: 0}, alloc: {2: Q.get('alloc') !== '0', 3: Q.get('alloc') !== '0'}};
  S.host = {version: '0.4.2-beta.1', dir: 'C:\\Games\\RE4', valid: true, msg: 'dir_ok', installing: false, lan: '192.168.1.23', demo: true};
  const r = demoState.recv, now = performance.now();
  if (r.stream) r.lastT = now;
  else { r.lastT = now - 12000; r.stopT = r.lastT; }     // ?stream=0: the phone stopped 12 s ago
  RX.hist = Array.from({length: 200}, (_, i) => (r.stream || i < 152) ? 27 + Math.sin(i / 3) * 2 + Math.sin(i * 1.7) * 1.5 + (i % 37 === 0 ? -9 : 0) : 0);   // seeded history so screenshots have a curve
  if (r.stream) RX.hist.push(...Array(40).fill(0).map((_, i) => 28 + Math.sin(i) * 1.5));
  demoTimer = setInterval(demoTick, 100);
}
function demoTick() {
  const d = demoState, r = d.recv, now = performance.now(), live = r.running && r.stream;
  if (live) { r.pacc += 0.1 * (58 + Math.sin(now / 900) * 4); r.facc += 0.1 * (29 + Math.sin(now / 700) * 2 - (Math.sin(now / 3100) > .96 ? 9 : 0));
    const p = Math.floor(r.pacc), f = Math.floor(r.facc); r.pacc -= p; r.facc -= f; r.packets += p; r.frames += f; if (f > 0) r.lastT = now; }
  const age = r.lastT ? Math.round(now - r.lastT) : 2147483647;
  const lines = [['S', 'demo:Example', d.owned ? 1 : 0, d.source, d.strength.toFixed(3), '已绑定：示例模型 / Example model', '示例模型 / Example model', '', '', d.compare, d.sweep].join('\t'),
    ['R', 1, r.running ? 1 : 0, 9000, '0.0.0.0', r.packets, r.frames, age, 0, 0, live ? 'RhyLive 接收中' : 'RhyLive'].join('\t'),
    ['C', d.cap.toFixed(2)].join('\t')];
  const ms = [[1, 'head', 159], [2, 'body', 5], [3, 'hair', 4]];
  for (const [i, n, c] of ms) { const ok = i === 1 || d.alloc[i]; lines.push(['M', i, n, c, ok ? c : 0, c, ok ? c : 0, ok ? 1 : 0].join('\t')); }
  for (const k of d.keys) {
    k.actual += (k.manual - k.actual) * 0.35; if (Math.abs(k.manual - k.actual) < 1e-4) k.actual = k.manual;   // game lags slightly behind
    const w = k.mesh === 1 || d.alloc[k.mesh];
    lines.push(['K', k.id, k.label, k.region, k.manual.toFixed(4), (d.compare ? 0 : k.actual).toFixed(4), w ? 1 : 0, k.mesh, k.group].join('\t'));
  }
  onSnap(lines.join('\n'), true);
}
function demoCommand(parts) {
  if (parts[0] !== 'cmd' || !demoState) return; const [, op, arg, value] = parts; const d = demoState;
  if (op === 'start') d.owned = true; else if (op === 'release') d.owned = false;
  else if (op === 'alloc') { if (arg) d.alloc[+arg] = true; else d.alloc = {2: true, 3: true}; }
  else if (op === 'receiver_start' || op === 'receiver_config') d.recv.running = true; else if (op === 'receiver_stop') d.recv.running = false;
  else if (op === 'cap_smooth') { const v = parseFloat(value); if (isFinite(v)) d.cap = clamp(v, 0.05, 1); }
  else if (!d.owned) return;
  else if (op === 'set') { const k = d.keys.find(x => x.id === arg); if (k) k.manual = clamp(+value, 0, 1); }
  else if (op === 'zero') d.keys.forEach(k => k.manual = 0);
  else if (op === 'source') d.source = +arg; else if (op === 'gain') d.strength = +arg;
  else if (op === 'compare') d.compare = +arg; else if (op === 'sweep') d.sweep = +arg;
}
function demoHost(action, arg) {
  if (action === 'validate') { S.host.dir = arg; S.host.valid = /re4/i.test(arg || ''); S.host.msg = S.host.valid ? 'dir_ok' : 'dir_bad'; renderSetup(); }
  else if (action === 'preset_save') toast(lang === 'zh' ? '演示模式：不写文件' : 'Demo: nothing written', 'ok');
  else if (action === 'preset_load') applyPreset(presetText());
}

/* ------------------------------------------------------------------ boot */
applyTheme(); applyLang();
if (DEMO) { demoInit(); if (HOST) hostAction('log', 'demo boot'); booted = true; if (Q.get('view') !== 'setup') switchView('bench', true); else renderSetup(); if (Q.get('page')) goPage(Q.get('page')); demoTick(); }
if (HOST) hostAction('ready');
/* demo-only storyboard: ?seek=<ms>&go=<page> freezes the tab switch at that time; ?rbseek=<ms>&drag=<id>:<v> freezes the read-back dot */
if (DEMO && Q.get('seek') != null) setTimeout(() => {
  const ms = +Q.get('seek'), target = Q.get('go'); const pill = $('#navPill'), page = $('#page');
  springs.clear(); document.getAnimations().forEach(a => a.finish()); const before = new Set(document.getAnimations());
  if (ms < 120) {
    S.page = target; markNav(false); pill.getAnimations().forEach(a => { a.pause(); a.currentTime = ms; });
    const a = anim(page, [{opacity: 1, transform: 'none'}, {opacity: 0, transform: 'translateY(-8px)'}], {duration: 120, easing: 'ease-in'}); a.pause(); a.currentTime = ms;
  } else {
    S.page = target; markNav(false); renderPage(true);
    for (const a of document.getAnimations()) { if (before.has(a)) continue; a.pause(); a.currentTime = pill.getAnimations().includes(a) ? ms : ms - 120; }
  }
}, 600);
if (DEMO && Q.get('rbseek') != null) setTimeout(() => {
  const ms = +Q.get('rbseek'); const [id, v] = Q.get('drag').split(':'); const k = S.byId.get(id), r = rows.get(id); if (!k || !r) return;
  clearInterval(demoTimer); springs.clear(); selectKey(id); r.slider.set(+v); r.num.value = fmt(+v); r.el.classList.add('sel');
  const from = k.actual, cur = +v + (from - +v) * Math.exp(-11 * ms / 1000); r.slider.rbs.apply(cur);
  r.slider.el.classList.toggle('drag', ms < 200);
}, 600);
/* demo-only capture hooks for motion frames: ?go=<page>&delay=ms, ?drag=<keyId>:<value>&delay=ms */
if (DEMO && Q.get('delay')) setTimeout(() => {
  if (Q.get('go')) goPage(Q.get('go'));
  if (Q.get('drag')) { const [id, v] = Q.get('drag').split(':'); const k = S.byId.get(id); selectKey(id); const r = rows.get(id); if (k && r) { r.slider.set(+v); setKey(k, +v, true); } }
}, +Q.get('delay'));
