"""PlusMinus Studio — Python 3 / Tk desktop front end."""
import json
import queue
import re
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, simpledialog, ttk
from engine import execute, check, find_runner

BG, PANEL, FIELD, LINE, FG, MUTED = '#171717', '#202020', '#141414', '#363636', '#e0e0e0', '#929292'
EXAMPLES = {
    'Arithmetic': ('print(2 + 3 * 4);\n', '14\n'),
    'Power': ('print(2 ^ 3 ^ 2);\n', '512\n'),
    'Variables': ('x = 10;\ny = x * 2;\nprint(y);\n', '20\n'),
    'Branch': ('x = 0;\nif (1 < 2) { x = 7; } else { x = 9; }\nprint(x);\n', '7\n'),
    'Loop': ('i = 0;\nwhile (i < 3) {\n    print(i);\n    i = i + 1;\n}\n', '0\n1\n2\n'),
}

class Studio:
    def __init__(self, root):
        self.root = root
        root.title('PlusMinus Studio')
        root.geometry('1240x800'); root.minsize(850, 560); root.configure(bg=BG)
        self.path = None; self.saved = ''; self.runner = find_runner()
        self.cases = []; self.history = []; self.busy = False; self.cancel = threading.Event(); self.events = queue.Queue()
        self.status = tk.StringVar(value='Ready'); self.file_label = tk.StringVar(value='Untitled.pm'); self.timeout = tk.StringVar(value='5'); self.cursor = tk.StringVar(value='Ln 1 : Col 1')
        self.style(); self.layout(); self.replace(EXAMPLES['Arithmetic'][0]); self.saved = self.source(); self.update_editor(); self.add_examples()
        self.bind_keys(); self.poll(); self.update_editor()
        root.protocol('WM_DELETE_WINDOW', self.close)

    def style(self):
        s = ttk.Style(); s.theme_use('clam')
        s.configure('.', background=PANEL, foreground=FG, fieldbackground=FIELD, bordercolor=LINE, font=('TkDefaultFont', 10))
        s.configure('TButton', background='#2b2b2b', foreground=FG, padding=(12, 7), relief='flat')
        s.map('TButton', background=[('active','#454545'),('disabled',PANEL)], foreground=[('disabled',MUTED)])
        s.configure('Primary.TButton', background='#dedede', foreground='#151515'); s.map('Primary.TButton',background=[('active','#ffffff'),('disabled',PANEL)], foreground=[('disabled',MUTED)])
        s.configure('TNotebook', background=PANEL, borderwidth=0); s.configure('TNotebook.Tab', background=PANEL, foreground=MUTED, padding=(13,9)); s.map('TNotebook.Tab',background=[('selected',FIELD)],foreground=[('selected',FG)])
        s.configure('Treeview', background=FIELD, fieldbackground=FIELD, foreground=FG, rowheight=27); s.configure('Treeview.Heading', background=PANEL, foreground=FG); s.map('Treeview',background=[('selected','#424242')])
        s.configure('TEntry', fieldbackground=FIELD, foreground=FG, insertcolor=FG)
        s.configure('TPanedwindow', background=LINE); s.configure('TCombobox',fieldbackground=FIELD,foreground=FG); s.map('TCombobox',fieldbackground=[('readonly',FIELD)],foreground=[('readonly',FG)])

    def label(self, parent, text='', **kw):
        return tk.Label(parent, text=text, bg=kw.pop('bg', PANEL), fg=kw.pop('fg', FG), **kw)

    def button(self, parent, text, command, primary=False):
        b = ttk.Button(parent, text=text, command=command, style='Primary.TButton' if primary else 'TButton'); b.pack(side='left', padx=(0,5)); return b

    def box(self, parent, readonly=False, height=8, wrap='none'):
        frame = tk.Frame(parent, bg=FIELD); frame.pack(fill='both', expand=True)
        t = tk.Text(frame, bg=FIELD, fg=FG, insertbackground=FG, selectbackground='#414141', font=('TkFixedFont',11), relief='flat', borderwidth=0, padx=12, pady=10, wrap=wrap, height=height, undo=not readonly)
        scroll = ttk.Scrollbar(frame, command=t.yview); scroll.pack(side='right',fill='y'); t.configure(yscrollcommand=scroll.set)
        if wrap == 'none':
            x = ttk.Scrollbar(frame, orient='horizontal',command=t.xview); x.pack(side='bottom',fill='x'); t.configure(xscrollcommand=x.set)
        t.pack(side='left',fill='both',expand=True)
        if readonly: t.configure(state='disabled')
        return t

    def layout(self):
        head = tk.Frame(self.root,bg=PANEL,padx=16,pady=12); head.pack(fill='x')
        self.label(head,'+/−  PLUSMINUS',font=('TkDefaultFont',15,'bold')).pack(side='left')
        self.label(head,'STUDIO / LOCAL WORKSPACE',fg=MUTED,font=('TkDefaultFont',9)).pack(side='left',padx=15)
        self.label(head,'C++ VM  ·  ISOLATED PROCESS',fg=MUTED).pack(side='right')
        toolbar=tk.Frame(self.root,bg=PANEL,padx=16,pady=8); toolbar.pack(fill='x')
        for name,fn in [('New',self.new),('Open',self.open),('Save',self.save),('Save as',lambda:self.save(True)),('Runner…',self.choose_runner)]: self.button(toolbar,name,fn)
        self.run_btn=self.button(toolbar,'Run  F5',lambda:self.start('run'),True)
        self.compile_btn=self.button(toolbar,'Compile  F6',lambda:self.start('compile'))
        self.stop_btn=self.button(toolbar,'Stop',self.stop); self.stop_btn.configure(state='disabled')
        self.label(toolbar,'Timeout (s)',fg=MUTED).pack(side='left',padx=(8,4)); ttk.Entry(toolbar,textvariable=self.timeout,width=5).pack(side='left')
        main=ttk.Panedwindow(self.root,orient='horizontal'); main.pack(fill='both',expand=True,padx=12,pady=10)
        side=tk.Frame(main,bg=PANEL,padx=10,pady=10,width=190); middle=tk.Frame(main,bg=PANEL); right=tk.Frame(main,bg=PANEL)
        main.add(side,weight=0); main.add(middle,weight=3); main.add(right,weight=2)
        self.label(side,'EXAMPLES',fg=MUTED,anchor='w').pack(fill='x',pady=(0,8))
        for name in EXAMPLES:
            ttk.Button(side,text=name,command=lambda n=name:self.load_example(n)).pack(fill='x',pady=2)
        self.label(side,'RUN HISTORY',fg=MUTED,anchor='w').pack(fill='x',pady=(24,5))
        self.history_box=tk.Listbox(side,bg=FIELD,fg=FG,selectbackground='#444444',relief='flat',height=10,exportselection=False); self.history_box.pack(fill='both',expand=True); self.history_box.bind('<Double-1>',self.restore_history)
        self.label(side,'Double-click to restore source',fg=MUTED,wraplength=160,justify='left').pack(anchor='w',pady=8)
        editorhead=tk.Frame(middle,bg=PANEL,padx=12,pady=10); editorhead.pack(fill='x'); self.label(editorhead,textvariable=self.file_label,font=('TkDefaultFont',11,'bold')).pack(side='left'); self.label(editorhead,textvariable=self.cursor,fg=MUTED).pack(side='right')
        editorframe=tk.Frame(middle,bg=FIELD); editorframe.pack(fill='both',expand=True)
        self.editor=self.box(editorframe,height=24); self.editor.configure(tabs=('32p',)); self.editor.tag_configure('keyword',foreground='#f5f5f5',font=('TkFixedFont',11,'bold')); self.editor.tag_configure('comment',foreground='#777777'); self.editor.tag_configure('number',foreground='#b1b1b1'); self.editor.tag_configure('string',foreground='#bcbcbc')
        self.editor.bind('<KeyRelease>',self.update_editor); self.editor.bind('<ButtonRelease-1>',self.update_editor); self.editor.bind('<Tab>',self.indent)
        self.label(middle,'F5 Run   ·   F6 Compile   ·   Ctrl+S Save   ·   Ctrl+F Find',fg=MUTED,anchor='w').pack(fill='x',padx=10,pady=8)
        self.tabs=ttk.Notebook(right); self.tabs.pack(fill='both',expand=True)
        self.views={}
        for key,title in [('output','Output'),('variables','Variables'),('bytecode','Bytecode'),('error','Diagnostics')]:
            frame=tk.Frame(self.tabs,bg=PANEL); self.tabs.add(frame,text=title); self.views[key]=self.box(frame,True,wrap='word' if key=='error' else 'none')
        tests=tk.Frame(self.tabs,bg=PANEL,padx=8,pady=8); self.tabs.add(tests,text='Tests'); self.test_tab=tests
        self.test_table=ttk.Treeview(tests,columns=('kind','status'),show='tree headings',height=7,selectmode='browse'); self.test_table.heading('#0',text='Test case'); self.test_table.heading('kind',text='Target'); self.test_table.heading('status',text='Status'); self.test_table.column('#0',width=135); self.test_table.column('kind',width=65); self.test_table.column('status',width=80); self.test_table.pack(fill='x'); self.test_table.bind('<Double-1>',self.load_case)
        row=tk.Frame(tests,bg=PANEL); row.pack(fill='x',pady=7)
        self.button(row,'Add',self.add_case); self.button(row,'Remove',self.remove_case); self.test_btn=self.button(row,'Run all',self.run_tests,True)
        row2=tk.Frame(tests,bg=PANEL); row2.pack(fill='x',pady=(0,8)); self.button(row2,'Import',self.import_cases); self.button(row2,'Export',self.export_cases)
        self.test_log=self.box(tests,True,wrap='word')
        bottom=tk.Frame(self.root,bg=PANEL,padx=14,pady=7); bottom.pack(fill='x'); self.label(bottom,textvariable=self.status,anchor='w').pack(side='left'); self.label(bottom,'PLUSMINUS STUDIO  /  01',fg=MUTED).pack(side='right')

    def bind_keys(self):
        for key, fn in [('<F5>',lambda:self.start('run')),('<F6>',lambda:self.start('compile')),('<Control-s>',self.save),('<Control-o>',self.open),('<Control-n>',self.new),('<Control-f>',self.find),('<Escape>',self.stop)]:
            self.root.bind(key,lambda e,f=fn:(f(),'break')[1])
    def source(self): return self.editor.get('1.0','end-1c')
    def replace(self,s): self.editor.delete('1.0','end'); self.editor.insert('1.0',s); self.editor.edit_reset(); self.update_editor()
    def write(self, widget, text): widget.configure(state='normal'); widget.delete('1.0','end'); widget.insert('1.0',text); widget.configure(state='disabled')
    def update_editor(self,event=None):
        line,col=self.editor.index('insert').split('.'); self.cursor.set(f'Ln {line} : Col {int(col)+1}')
        self.file_label.set((self.path.name if self.path else 'Untitled.pm')+(' *' if self.source()!=self.saved else ''))
        text=self.source()
        for tag in ('keyword','comment','number','string'): self.editor.tag_remove(tag,'1.0','end')
        pattern=r'(?P<comment>//[^\n]*|/\*[\s\S]*?\*/)|(?P<string>"[^"\n]*")|(?P<keyword>\b(?:if|else|while|func|proc|return|go|sync|lock|unlock|print|len|resize|sin|cos|pow|fact|multifact)\b)|(?P<number>\b\d+(?:\.\d+)?\b)'
        for match in re.finditer(pattern,text): self.editor.tag_add(match.lastgroup,f'1.0+{match.start()}c',f'1.0+{match.end()}c')
    def indent(self,event): self.editor.insert('insert','    '); self.update_editor(); return 'break'
    def find(self):
        needle=simpledialog.askstring('Find','Search source:',parent=self.root)
        if needle:
            pos=self.editor.search(needle,'insert+1c',stopindex='end') or self.editor.search(needle,'1.0',stopindex='end')
            if pos: self.editor.tag_remove('sel','1.0','end'); self.editor.tag_add('sel',pos,f'{pos}+{len(needle)}c'); self.editor.mark_set('insert',pos); self.editor.see(pos); self.editor.focus_set()
            else: self.status.set('No match')
    def confirm(self):
        if self.source()==self.saved: return True
        choice=messagebox.askyesnocancel('Unsaved changes','Save the current source?',parent=self.root)
        return False if choice is None else self.save() if choice else True
    def new(self):
        if self.confirm(): self.path=None; self.saved=''; self.replace('')
    def open(self):
        if not self.confirm(): return
        path=filedialog.askopenfilename(filetypes=[('PlusMinus','*.pm'),('All files','*')]);
        if path:
            try: data=Path(path).read_text(encoding='utf-8-sig'); self.path=Path(path); self.saved=data; self.replace(data)
            except (OSError,UnicodeError) as exc: messagebox.showerror('Open failed',str(exc))
    def save(self,save_as=False):
        path=str(self.path) if self.path and not save_as else filedialog.asksaveasfilename(defaultextension='.pm',filetypes=[('PlusMinus','*.pm'),('All files','*')])
        if not path: return False
        try: Path(path).write_text(self.source(),encoding='utf-8'); self.path=Path(path); self.saved=self.source(); self.update_editor(); return True
        except OSError as exc: messagebox.showerror('Save failed',str(exc)); return False
    def load_example(self,name):
        if self.confirm(): self.path=None; self.saved=EXAMPLES[name][0]; self.replace(self.saved)
    def choose_runner(self):
        p=filedialog.askopenfilename(title='Select compiled plusminus_runner');
        if p: self.runner=Path(p); self.status.set(f'Runner: {self.runner.name}')
    def ready(self):
        if self.busy: return None
        if not self.runner or not self.runner.is_file(): self.runner=find_runner()
        if not self.runner: messagebox.showerror('Runner missing','Build ide/build first (see ide/README.md), or select the executable using Runner…'); return None
        try:
            value=float(self.timeout.get()); assert 0.1<=value<=300
        except (ValueError,AssertionError): messagebox.showerror('Invalid timeout','Enter a number from 0.1 to 300 seconds.'); return None
        self.busy=True; self.cancel.clear()
        for b in (self.run_btn,self.compile_btn,self.test_btn): b.configure(state='disabled')
        self.stop_btn.configure(state='normal'); self.status.set('Running…'); return self.runner,value
    def start(self,mode):
        config=self.ready()
        if config is None:return
        path,timeout=config; source=self.source()
        def worker():
            report=execute(path,source,mode,timeout,self.cancel); self.events.put(('run',mode,source,report)); self.events.put(('done',))
        threading.Thread(target=worker,daemon=True).start()
    def stop(self):
        if self.busy: self.cancel.set(); self.status.set('Stopping…')
    def poll(self):
        try:
            while True:
                event=self.events.get_nowait(); kind=event[0]
                if kind=='run': self.show_run(*event[1:])
                elif kind=='case':
                    _,index,case,report=event; passed=check(case,report); self.test_table.set(str(index),'status','PASS' if passed else 'FAIL')
                    self.test_messages.append(f"{'PASS' if passed else 'FAIL'}  {case['name']}  {report['ms']} ms\nExpected: {case['expected']!r}\nActual: {report[case['kind']]!r}\n{report['error']}\n"); self.write(self.test_log,'\n'.join(self.test_messages))
                elif kind=='done':
                    self.busy=False
                    for b in (self.run_btn,self.compile_btn,self.test_btn): b.configure(state='normal')
                    self.stop_btn.configure(state='disabled'); self.status.set('Stopped' if self.cancel.is_set() else self.status.get().replace('Running…','Finished'))
                elif kind=='summary': self.status.set(f'{event[1]} / {event[2]} passed')
        except queue.Empty: pass
        self.root.after(60,self.poll)
    def show_run(self,mode,source,report):
        output=report['output']; result=report['result']
        self.write(self.views['output'],output+('\n' if output else '')+(f'VM result: {result}\n' if result else ''))
        for key in ('variables','bytecode','error'): self.write(self.views[key],report[key])
        self.tabs.select(3 if not report['ok'] else 2 if mode=='compile' else 0); state='OK' if report['ok'] else 'ERROR'; self.status.set(f'{mode.upper()}  {state}  ·  {report["ms"]} ms')
        self.history.insert(0,source); self.history_box.insert(0,f'{state}  {mode}  {report["ms"]} ms')
        if len(self.history)>50:self.history.pop(); self.history_box.delete(50)
    def restore_history(self,event):
        selection=self.history_box.curselection()
        if selection and self.confirm(): self.path=None; self.saved=''; self.replace(self.history[selection[0]])
    def refresh_cases(self):
        self.test_table.delete(*self.test_table.get_children())
        for i,c in enumerate(self.cases): self.test_table.insert('','end',iid=str(i),text=c['name'],values=(c['kind'],'—'))
    def add_examples(self):
        self.cases=[dict(name=n,source=s,kind='output',expected=e) for n,(s,e) in EXAMPLES.items()]; self.refresh_cases()
    def add_case(self):
        if self.busy:return
        dialog=tk.Toplevel(self.root); dialog.title('New test case'); dialog.configure(bg=PANEL); dialog.transient(self.root); dialog.grab_set(); dialog.geometry('480x300')
        self.label(dialog,'Name',anchor='w').pack(fill='x',padx=12,pady=(12,2)); name=ttk.Entry(dialog); name.pack(fill='x',padx=12); name.insert(0,f'Test {len(self.cases)+1}')
        kind=ttk.Combobox(dialog,values=('output','result'),state='readonly'); kind.set('output'); kind.pack(fill='x',padx=12,pady=8)
        self.label(dialog,'Expected value (exact, including final newline for print)',anchor='w').pack(fill='x',padx=12)
        expected=self.box(dialog,wrap='word'); source=self.source()
        def commit():
            if not name.get().strip():return
            self.cases.append(dict(name=name.get(),source=source,kind=kind.get(),expected=expected.get('1.0','end-1c'))); self.refresh_cases(); dialog.destroy()
        ttk.Button(dialog,text='Add current source',command=commit).pack(pady=8)
    def remove_case(self):
        selection=self.test_table.selection()
        if not self.busy and selection: self.cases.pop(int(selection[0])); self.refresh_cases()
    def load_case(self,event):
        selection=self.test_table.selection()
        if selection and self.confirm(): self.path=None; self.saved=''; self.replace(self.cases[int(selection[0])]['source'])
    def import_cases(self):
        if self.busy:return
        path=filedialog.askopenfilename(filetypes=[('Test suite','*.json')]);
        if not path:return
        try:
            cases=json.loads(Path(path).read_text(encoding='utf-8'))
            if not isinstance(cases,list) or not all(isinstance(c,dict) and all(isinstance(c.get(k),str) for k in ('name','source','kind','expected')) and c['kind'] in ('output','result') for c in cases): raise ValueError('Invalid test suite')
            self.cases=cases; self.refresh_cases()
        except (OSError,ValueError) as exc: messagebox.showerror('Import failed',str(exc))
    def export_cases(self):
        path=filedialog.asksaveasfilename(defaultextension='.json',filetypes=[('Test suite','*.json')]);
        if path:
            try: Path(path).write_text(json.dumps(self.cases,ensure_ascii=False,indent=2),encoding='utf-8')
            except OSError as exc: messagebox.showerror('Export failed',str(exc))
    def run_tests(self):
        if not self.cases:return
        config=self.ready()
        if config is None:return
        path,timeout=config; cases=list(self.cases); self.refresh_cases(); self.test_messages=[]; self.write(self.test_log,''); self.tabs.select(self.test_tab)
        def worker():
            passed=0
            for i,case in enumerate(cases):
                if self.cancel.is_set():break
                report=execute(path,case['source'],'run',timeout,self.cancel); passed+=int(check(case,report)); self.events.put(('case',i,case,report))
            self.events.put(('summary',passed,len(cases))); self.events.put(('done',))
        threading.Thread(target=worker,daemon=True).start()
    def close(self):
        if self.busy: self.stop(); self.status.set('Stopping; close again once finished'); return
        if self.confirm(): self.root.destroy()

def bundled_self_test():
    """Headless check of the bundled runner, used in release CI."""
    runner = find_runner()
    if runner is None:
        return 2
    for source, expected in (("print(2 + 3 * 4);", "14\n"),
                             ("x = 10; print(x);", "10\n")):
        report = execute(runner, source, 'run', 5.0)
        if not report['ok'] or report['output'].replace('\r\n', '\n') != expected:
            return 3
    return 0


if __name__=='__main__':
    import sys
    if '--self-test' in sys.argv:
        sys.exit(bundled_self_test())
    root=tk.Tk(); Studio(root); root.mainloop()
