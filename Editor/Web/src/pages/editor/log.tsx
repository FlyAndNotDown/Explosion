import { useEffect, useRef, useState } from 'react';
import { Button } from '@heroui/button';
import { Chip } from '@heroui/chip';
import { Select, SelectItem } from '@heroui/react';
import { QWebChannel } from '@/qwebchannel';

interface LogEntryInfo {
  time: string;
  tag: string;
  level: string;
  content: string;
}

const levelColors: Record<string, 'default' | 'primary' | 'warning' | 'danger'> = {
  verbose: 'default',
  info: 'primary',
  warning: 'warning',
  error: 'danger'
};

export default function LogPage() {
  const [entries, setEntries] = useState(Array<LogEntryInfo>);
  const [levelFilter, setLevelFilter] = useState('all');
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    new QWebChannel(window.qt.webChannelTransport, (channel: QWebChannel): void => {
      window.backend = channel.objects.backend;
      window.backend.GetHistory((history: Array<LogEntryInfo>) => setEntries(history ?? []));
      window.backend.EntryAppended.connect((entry: LogEntryInfo) => {
        setEntries((prev) => [...prev.slice(-1999), entry]);
      });
    });
  }, []);

  useEffect(() => {
    scrollRef.current?.scrollTo({ top: scrollRef.current.scrollHeight });
  }, [entries]);

  const visibleEntries = levelFilter === 'all' ? entries : entries.filter((entry) => entry.level === levelFilter);

  return (
    <div className='flex flex-col h-screen p-2 gap-2 bg-background text-foreground'>
      <div className='flex gap-2 items-center'>
        <Select
          size='sm'
          className='w-40'
          aria-label='level filter'
          selectedKeys={new Set([levelFilter])}
          onSelectionChange={(keys) => setLevelFilter(Array.from(keys as Set<string>)[0] ?? 'all')}
        >
          <SelectItem key='all'>All</SelectItem>
          <SelectItem key='verbose'>Verbose</SelectItem>
          <SelectItem key='info'>Info</SelectItem>
          <SelectItem key='warning'>Warning</SelectItem>
          <SelectItem key='error'>Error</SelectItem>
        </Select>
        <Button size='sm' variant='flat' onPress={() => setEntries([])}>Clear</Button>
      </div>
      <div ref={scrollRef} className='flex-1 overflow-y-auto font-mono text-xs'>
        {visibleEntries.map((entry, index) => (
          <div key={index} className='flex gap-2 items-center py-0.5 border-b border-default-100'>
            <span className='text-default-400 shrink-0'>{entry.time}</span>
            <Chip size='sm' variant='flat' color={levelColors[entry.level] ?? 'default'}>{entry.tag}</Chip>
            <span className='whitespace-pre-wrap break-all'>{entry.content}</span>
          </div>
        ))}
      </div>
    </div>
  );
}
